/**
 * @file OpenRGB.cpp
 * @brief Implementation of the high-level Gorgon OpenRGB subsystem.
 *
 * Threading model
 * ---------------
 * - `client_mtx` serializes every use of the network client (worker tick, Flush,
 *   RestoreAll, mode changes, reconnection).
 * - `state_mtx` guards all in-memory device state (LED colors, flags, registry).
 *   Caller threads only ever take `state_mtx`, update colors, mark the device
 *   dirty and return; they never wait for network I/O.
 * - Lock order is always `client_mtx` -> `state_mtx`.
 * - The worker thread wakes up every 1/FPS seconds, composes the final colors
 *   (this is the place where animations will be progressed in the future),
 *   diffs them against what was last sent and flushes only what changed. All
 *   network I/O happens outside of `state_mtx`.
 * - The worker is terminated cleanly through an atexit handler (like the audio
 *   loop) or by Shutdown(); hardware is restored after the worker is joined so
 *   that no late frame can overwrite the restored state.
 */

#include "../OpenRGB.h"
#include "OpenRGBClient.h"

#include "../../Graphics/Bitmap.h"
#include "../../Input/Keyboard.h"
#include "../../Main.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <map>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>

namespace Gorgon {
    extern bool exiting;
}

namespace Gorgon :: Devices :: OpenRGB {

    namespace LL = LowLevel;
    namespace K  = Input::Keyboard::Keycodes;

    Utils::Logger Logger("OpenRGB");
    Event<void> DeviceConnectedEvent;
    Event<void, const std::string &> ErrorEvent;

    namespace {
        void fireDeviceConnected() {
            Gorgon::RegisterOnce([]() {
                DeviceConnectedEvent();
            });
        }

        void fireError(const std::string &msg) {
            if(!msg.empty()) {
                Gorgon::RegisterOnce([msg]() {
                    ErrorEvent(msg);
                });
            }
        }
    }

    // -------------------------------------------------------------
    // Implementation structures
    // -------------------------------------------------------------

    struct Device::Impl {
        int id = -1;
        bool connected = false;
        std::string name, vendor, description, version, serial, location;
        DeviceType type = DeviceType::Unknown;

        std::vector<LED>  leds;      ///< Public LED metadata; `color` holds the desired (base) color
        std::vector<Zone> zones;

        std::vector<LL::Mode> modes;
        int active_mode = 0;
        int direct_mode = -1;

        // Snapshot taken on discovery, used for restoration
        int                   orig_mode = 0;
        LL::Mode              orig_mode_data;
        std::vector<LL::Color> orig_colors;
        bool                  orig_direct = false;

        // Keyboard info
        bool is_keyboard = false;
        int  matrix_zone = -1;
        std::unordered_map<int, std::vector<int>> keymap;

        // Runtime state (state_mtx)
        bool     touched     = false; ///< Software has modified this device; restore is needed
        bool     engaged     = false; ///< Device is in direct mode driven by us
        bool     dirty       = false;
        bool     unsupported = false;
        uint64_t revision    = 0;     ///< Incremented on every write, used to detect concurrent writes

        // Only accessed with client_mtx held
        std::vector<LL::Color> last_sent;
    };

    struct Zone::Impl {
        int index = 0;
        std::string name;
        ZoneType type = ZoneType::Single;
        int led_count = 0;
        int led_offset = 0;

        int width = 0, height = 0;
        std::vector<int> matrix; ///< width*height zone-local LED indices, -1 if unpopulated

        std::weak_ptr<Device::Impl> dev;
    };

    /// Grants the implementation access to private state of the public classes.
    struct Access {
        using DImpl = Device::Impl;
        using ZImpl = Zone::Impl;
        using PImpl = Progressbar::Impl;

        static std::shared_ptr<DImpl> impl(const Device &d)      { return d.impl; }
        static std::shared_ptr<ZImpl> impl(const Zone &z)        { return z.impl; }
        static std::shared_ptr<PImpl> impl(const Progressbar &p) { return p.impl; }

        static void setImpl(Zone &z, std::shared_ptr<ZImpl> i)   { z.impl = std::move(i); }

        static Device         *newDevice(std::shared_ptr<DImpl> i)   { return new Device(std::move(i)); }
        static KeyboardDevice *newKeyboard(std::shared_ptr<DImpl> i) { return new KeyboardDevice(std::move(i)); }
    };

    struct Progressbar::Impl {
        enum class Target { None, AllKeyboards, Keyboard, Zone };

        Target target = Target::None;
        KeyGroup keys;
        bool use_keys = false;
        std::weak_ptr<Access::DImpl> device;
        std::shared_ptr<Access::ZImpl> zone;

        Graphics::RGBA color = Graphics::RGBA(0xff00ff00u);
        Graphics::RGBA bg    = Graphics::RGBA(0x00000000u);
        std::vector<LevelColor> levels;
        bool fill = true;
        Float ratio = 0;
        bool active = false;

        std::map<std::pair<std::shared_ptr<Access::DImpl>, int>, Graphics::RGBA> underlay;
    };

    using DImpl = Access::DImpl;
    using ZImpl = Access::ZImpl;
    using PImpl = Access::PImpl;

    namespace {

        using Clock = std::chrono::steady_clock;

        struct State {
            std::mutex state_mtx, client_mtx, init_mtx, wake_mtx;
            std::condition_variable wake;

            LL::Client client;
            std::vector<std::unique_ptr<Device>> devices; ///< Registry; stable addresses until Shutdown

            std::thread worker;
            std::atomic<bool> running{false};
            std::atomic<bool> initialized{false};
            std::atomic<bool> connected{false};
            std::atomic<int>  fps{30};

            std::string host = "127.0.0.1";
            uint16_t    port = 6742;

            Clock::time_point next_retry{};      ///< Lazy initialization cooldown
            Clock::time_point next_reconnect{};  ///< Worker reconnection cooldown
            bool atexit_registered = false;
        };

        State &S() {
            static State s;
            return s;
        }

        // ---------------------------------------------------------
        // Utilities
        // ---------------------------------------------------------

        std::string lower(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return s;
        }

        /// Lowercases, removes the "Key:" prefix and whitespace for tolerant name matching
        std::string normalize(const std::string &name) {
            std::string s = lower(name);
            if(s.rfind("key:", 0) == 0)
                s = s.substr(4);

            std::string r;
            for(char c : s) {
                if(c != ' ' && c != '_' && c != '\t')
                    r += c;
            }

            return r;
        }

        LL::Color wire(const Graphics::RGBA &c) {
            return LL::Color(c.R, c.G, c.B, 0);
        }

        Graphics::RGBA opaque(const Graphics::RGBA &c) {
            return Graphics::RGBA(c.R, c.G, c.B, Byte(255));
        }

        Graphics::RGBA lerp(const Graphics::RGBA &a, const Graphics::RGBA &b, float t) {
            t = Clamp(t, 0.f, 1.f);
            auto mix = [&](Byte x, Byte y) { return Byte(x + (y - x) * t + 0.5f); };

            return Graphics::RGBA(mix(a.R, b.R), mix(a.G, b.G), mix(a.B, b.B), Byte(255));
        }

        const Graphics::RGBA transparent = Graphics::RGBA(Byte(0), Byte(0), Byte(0), Byte(0));

        // ---------------------------------------------------------
        // Key name mapping
        // ---------------------------------------------------------

        /// Candidate (normalized) LED names for every Gorgon key
        const std::map<int, std::vector<std::string>> &KeyNames() {
            static const std::map<int, std::vector<std::string>> names = []() {
                std::map<int, std::vector<std::string>> m;

                for(char c = 'a'; c <= 'z'; c++)
                    m[c - 'a' + 'A'] = {std::string(1, c)};

                for(char c = '0'; c <= '9'; c++)
                    m[c] = {std::string(1, c)};

                for(int i = 1; i <= 12; i++)
                    m[K::F1 + i - 1] = {"f" + std::to_string(i)};

                for(int i = 0; i <= 9; i++) {
                    auto n = std::to_string(i);
                    m[K::Numpad_0 + i] = {"numberpad" + n, "numpad" + n, "keypad" + n};
                }

                m[K::Numpad_Decimal] = {"numberpad.", "numpad.", "keypad.", "numberpaddecimal", "numpaddecimal"};
                m[K::Numpad_Div]     = {"numberpad/", "numpad/", "keypad/", "numberpaddivide", "numpaddivide"};
                m[K::Numpad_Mult]    = {"numberpad*", "numpad*", "keypad*", "numberpadmultiply", "numpadmultiply"};
                m[K::Numpad_Plus]    = {"numberpad+", "numpad+", "keypad+", "numberpadplus", "numpadplus"};
                m[K::Numpad_Minus]   = {"numberpad-", "numpad-", "keypad-", "numberpadminus", "numpadminus"};
                m[K::Numpad_Enter]   = {"numberpadenter", "numpadenter", "keypadenter"};

                m[K::Escape]    = {"escape", "esc"};
                m[K::Enter]     = {"enter", "return"};
                m[K::Tab]       = {"tab"};
                m[K::Backspace] = {"backspace"};
                m[K::Space]     = {"space", "spacebar"};

                m[K::Shift]    = {"leftshift", "shift"};
                m[K::RShift]   = {"rightshift"};
                m[K::Control]  = {"leftcontrol", "leftctrl", "control", "ctrl"};
                m[K::RControl] = {"rightcontrol", "rightctrl"};
                m[K::Alt]      = {"leftalt", "alt"};
                m[K::RAlt]     = {"rightalt", "altgr"};
                m[K::Meta]     = {"leftwindows", "leftwin", "leftmeta", "leftsuper", "leftcommand", "windows", "meta", "super"};
                m[K::RMeta]    = {"rightwindows", "rightwin", "rightmeta", "rightsuper", "rightcommand"};

                m[K::Home]        = {"home"};
                m[K::End]         = {"end"};
                m[K::Insert]      = {"insert"};
                m[K::Delete]      = {"delete"};
                m[K::PageUp]      = {"pageup"};
                m[K::PageDown]    = {"pagedown"};
                m[K::PrintScreen] = {"printscreen", "prtsc", "printscr"};
                m[K::Pause]       = {"pause/break", "pausebreak", "pause", "break"};
                m[K::Menu]        = {"menu", "contextmenu", "application"};
                m[K::CapsLock]    = {"capslock"};
                m[K::Numlock]     = {"numberlock", "numlock"};
                m[K::ScrollLock]  = {"scrolllock"};

                m[K::Left]  = {"leftarrow", "left"};
                m[K::Up]    = {"uparrow", "up"};
                m[K::Right] = {"rightarrow", "right"};
                m[K::Down]  = {"downarrow", "down"};

                return m;
            }();

            return names;
        }

        std::vector<Input::Key> AllKnownKeys() {
            std::vector<Input::Key> keys;
            for(auto &p : KeyNames())
                keys.push_back(p.first);

            return keys;
        }

        // ---------------------------------------------------------
        // State helpers. Callers must hold state_mtx unless noted.
        // ---------------------------------------------------------

        void touch(DImpl &d) {
            d.touched = true;
            d.dirty   = true;
            d.revision++;
        }

        void setLED(DImpl &d, int idx, const Graphics::RGBA &c) {
            if(idx < 0 || idx >= (int)d.leds.size())
                return;

            auto col = opaque(c);
            if(d.touched && d.leds[idx].color == col)
                return;

            d.leds[idx].color = col;
            touch(d);
        }

        Graphics::RGBA getLED(const DImpl &d, int idx) {
            if(idx < 0 || idx >= (int)d.leds.size())
                return transparent;

            return d.leds[idx].color;
        }

        /// Composes the final colors of the device. Animations (blink, breathing) will
        /// be applied on top of the base colors here in the future.
        std::vector<LL::Color> compose(const DImpl &d) {
            std::vector<LL::Color> out;
            out.reserve(d.leds.size());

            for(auto &l : d.leds)
                out.push_back(wire(l.color));

            return out;
        }

        bool isDirectMode(const LL::Mode &m) {
            return (m.flags & LL::ModeFlags::HasPerLedColor) != 0;
        }

        std::vector<std::shared_ptr<DImpl>> deviceList() {
            std::vector<std::shared_ptr<DImpl>> list;
            for(auto &d : S().devices)
                list.push_back(Access::impl(*d));

            return list;
        }

        // ---------------------------------------------------------
        // Enumeration
        // ---------------------------------------------------------

        void buildDevice(int id, const LL::Controller &c) {
            auto d = std::make_shared<DImpl>();

            d->id          = id;
            d->connected   = true;
            d->name        = c.name;
            d->vendor      = c.vendor;
            d->description = c.description;
            d->version     = c.version;
            d->serial      = c.serial;
            d->location    = c.location;
            d->type        = (c.type >= LL::DeviceType::Motherboard && c.type <= LL::DeviceType::Unknown) ?
                                static_cast<DeviceType>(static_cast<int>(c.type)) : DeviceType::Unknown;
            d->modes       = c.modes;

            d->orig_mode = c.active_mode;
            d->active_mode = c.active_mode;
            if(c.active_mode >= 0 && c.active_mode < (int)c.modes.size()) {
                d->orig_mode_data = c.modes[c.active_mode];
                d->orig_direct = isDirectMode(d->orig_mode_data) || d->orig_mode_data.name == "Direct";
            }
            d->orig_colors = c.colors;

            for(int i = 0; i < (int)c.modes.size(); i++) {
                if(c.modes[i].name == "Direct") {
                    d->direct_mode = i;
                    break;
                }
            }
            if(d->direct_mode == -1) {
                for(int i = 0; i < (int)c.modes.size(); i++) {
                    if(isDirectMode(c.modes[i])) {
                        d->direct_mode = i;
                        break;
                    }
                }
            }

            // LEDs: devices not in direct mode start from black as their registers are not meaningful
            bool keyleds = false;
            for(size_t i = 0; i < c.leds.size(); i++) {
                LED led;
                led.index = (int)i;
                led.name  = c.leds[i].name;
                if(d->orig_direct && i < c.colors.size()) {
                    auto col = c.colors[i];
                    led.color = Graphics::RGBA(col.R(), col.G(), col.B(), Byte(255));
                }
                else {
                    led.color = Graphics::RGBA(Byte(0), Byte(0), Byte(0), Byte(255));
                }

                if(lower(led.name).rfind("key:", 0) == 0)
                    keyleds = true;

                d->leds.push_back(led);
            }

            d->is_keyboard = d->type == DeviceType::Keyboard || keyleds;

            // Registry object
            std::unique_ptr<Device> dev;
            if(d->is_keyboard)
                dev.reset(Access::newKeyboard(d));
            else
                dev.reset(Access::newDevice(d));

            // Zones
            int offset = 0;
            for(size_t i = 0; i < c.zones.size(); i++) {
                auto &lz = c.zones[i];
                auto type = (lz.type >= LL::ZoneType::Single && lz.type <= LL::ZoneType::Matrix) ?
                                static_cast<ZoneType>(static_cast<int>(lz.type)) : ZoneType::Linear;

                Zone zone((int)i, lz.name, type, (int)lz.leds_count, offset, *dev);

                if(type == ZoneType::Matrix && lz.matrix_map.width > 0 && lz.matrix_map.height > 0) {
                    auto zi = Access::impl(zone);
                    zi->width  = (int)lz.matrix_map.width;
                    zi->height = (int)lz.matrix_map.height;
                    zi->matrix.assign((size_t)zi->width * zi->height, -1);

                    for(int y = 0; y < zi->height; y++) {
                        for(int x = 0; x < zi->width; x++) {
                            auto v = lz.matrix_map.Get(y, x);
                            if(v != 0xFFFFFFFFu && v < lz.leds_count)
                                zi->matrix[(size_t)y * zi->width + x] = (int)v;
                        }
                    }

                    if(d->matrix_zone == -1)
                        d->matrix_zone = (int)i;
                }

                d->zones.push_back(zone);
                offset += (int)lz.leds_count;
            }

            // Key map
            if(d->is_keyboard) {
                std::unordered_map<std::string, std::vector<int>> byname;
                for(size_t i = 0; i < d->leds.size(); i++)
                    byname[normalize(d->leds[i].name)].push_back((int)i);

                for(auto &p : KeyNames()) {
                    for(auto &cand : p.second) {
                        auto it = byname.find(cand);
                        if(it != byname.end()) {
                            d->keymap[p.first] = it->second;
                            break;
                        }
                    }
                }
            }

            S().devices.push_back(std::move(dev));
        }

        /// Re-enumerates controllers from the OpenRGB server and reconciles them with
        /// existing device objects. Missing devices are detached (never deleted);
        /// reconnected devices have their state restored. client_mtx must be held.
        void enumerateControllersLocked() {
            auto &s = S();
            if(!s.client.IsConnected())
                return;

            int count = s.client.RequestControllerCount();
            if(count < 0)
                return;

            std::vector<LL::Controller> ctrls;
            ctrls.reserve(count);
            for(int i = 0; i < count; i++) {
                LL::Controller c;
                if(s.client.RequestControllerData((uint32_t)i, c))
                    ctrls.push_back(std::move(c));
                else
                    Logger << "Cannot read OpenRGB device " << i << ": " << s.client.GetLastError();
            }

            bool any_newly_connected = false;

            {
                std::lock_guard<std::mutex> sg(s.state_mtx);

                std::vector<bool> dev_matched(s.devices.size(), false);
                std::vector<bool> ctrl_matched(ctrls.size(), false);

                // Multi-pass matching heuristic:
                auto match_pass = [&](auto predicate) {
                    for(size_t i = 0; i < ctrls.size(); i++) {
                        if(ctrl_matched[i])
                            continue;

                        for(size_t j = 0; j < s.devices.size(); j++) {
                            if(dev_matched[j])
                                continue;

                            auto d = Access::impl(*s.devices[j]);
                            if(predicate(ctrls[i], d)) {
                                ctrl_matched[i] = true;
                                dev_matched[j] = true;

                                bool was_connected = d->connected;
                                d->connected   = true;
                                d->id          = (int)i;
                                d->location    = ctrls[i].location;
                                d->modes       = ctrls[i].modes;
                                d->active_mode = ctrls[i].active_mode;

                                // Recompute direct mode index
                                d->direct_mode = -1;
                                for(int m = 0; m < (int)ctrls[i].modes.size(); m++) {
                                    if(ctrls[i].modes[m].name == "Direct") {
                                        d->direct_mode = m;
                                        break;
                                    }
                                }
                                if(d->direct_mode == -1) {
                                    for(int m = 0; m < (int)ctrls[i].modes.size(); m++) {
                                        if(isDirectMode(ctrls[i].modes[m])) {
                                            d->direct_mode = m;
                                            break;
                                        }
                                    }
                                }

                                // Snapshot current hardware baseline for restoration
                                d->orig_mode = ctrls[i].active_mode;
                                if(ctrls[i].active_mode >= 0 && ctrls[i].active_mode < (int)ctrls[i].modes.size()) {
                                    d->orig_mode_data = ctrls[i].modes[ctrls[i].active_mode];
                                    d->orig_direct = isDirectMode(d->orig_mode_data) || d->orig_mode_data.name == "Direct";
                                }
                                d->orig_colors = ctrls[i].colors;

                                if(d->touched) {
                                    // Hardware reconnection state restoration:
                                    // Software had set colors before disconnect. Re-engage and push state.
                                    d->dirty = true;
                                    d->engaged = false;
                                    d->last_sent.clear();
                                    d->revision++;
                                    Logger << "Restoring lighting state for reconnected device: " << d->name << " (id " << d->id << ")";
                                }
                                else if(d->orig_direct) {
                                    for(size_t k = 0; k < d->leds.size() && k < ctrls[i].colors.size(); k++) {
                                        auto col = ctrls[i].colors[k];
                                        d->leds[k].color = Graphics::RGBA(col.R(), col.G(), col.B(), Byte(255));
                                    }
                                }

                                if(!was_connected)
                                    any_newly_connected = true;

                                break;
                            }
                        }
                    }
                };

                // Pass 1: Match by non-empty serial and name
                match_pass([](const LL::Controller &c, const std::shared_ptr<DImpl> &d) {
                    return !c.serial.empty() && c.serial == d->serial && c.name == d->name;
                });

                // Pass 2: Match by non-empty location and name
                match_pass([](const LL::Controller &c, const std::shared_ptr<DImpl> &d) {
                    return !c.location.empty() && c.location == d->location && c.name == d->name;
                });

                // Pass 3: Match by name, vendor, device type, LED count and zone count
                match_pass([](const LL::Controller &c, const std::shared_ptr<DImpl> &d) {
                    return c.name == d->name && c.vendor == d->vendor &&
                           static_cast<int32_t>(c.type) == static_cast<int32_t>(d->type) &&
                           c.leds.size() == d->leds.size() &&
                           c.zones.size() == d->zones.size();
                });

                // Pass 4: Match by name and LED count
                match_pass([](const LL::Controller &c, const std::shared_ptr<DImpl> &d) {
                    return c.name == d->name && c.leds.size() == d->leds.size();
                });

                // Mark any unmatched existing devices as detached (never deleted)
                for(size_t j = 0; j < s.devices.size(); j++) {
                    if(!dev_matched[j]) {
                        auto d = Access::impl(*s.devices[j]);
                        if(d->connected) {
                            Logger << "Device detached: " << d->name << " (was id " << d->id << ")";
                            d->connected = false;
                            d->id = -1;
                            d->engaged = false;
                            d->last_sent.clear();
                        }
                    }
                }

                // Add any unmatched active controllers as brand new devices
                for(size_t i = 0; i < ctrls.size(); i++) {
                    if(!ctrl_matched[i]) {
                        buildDevice((int)i, ctrls[i]);
                        any_newly_connected = true;
                    }
                }
            }

            if(any_newly_connected)
                fireDeviceConnected();
        }

        // ---------------------------------------------------------
        // Network operations. client_mtx must be held.
        // ---------------------------------------------------------

        bool handshake() {
            auto &s = S();
            if(s.client.RequestProtocolVersion(LL::OPENRGB_SDK_PROTOCOL_VERSION) == 0 && !s.client.IsConnected())
                return false;

            s.client.SetClientName("Gorgon");
            return true;
        }

        /// Restores devices modified by software to their snapshot state.
        void restoreLocked() {
            auto &s = S();
            if(!s.client.IsConnected())
                return;

            std::vector<std::shared_ptr<DImpl>> list;
            {
                std::lock_guard<std::mutex> sg(s.state_mtx);
                for(auto &d : deviceList()) {
                    if(d->touched && d->connected && d->id >= 0)
                        list.push_back(d);
                }
            }

            for(auto &d : list) {
                // Effect modes must have their direct registers cleared first, otherwise
                // the direct colors keep overriding the effect on some hardware.
                std::vector<LL::Color> colors;
                if(d->orig_direct && d->orig_colors.size() == d->leds.size())
                    colors = d->orig_colors;
                else
                    colors.assign(d->leds.size(), LL::Color(0, 0, 0, 0));

                s.client.UpdateLEDs((uint32_t)d->id, colors);
                s.client.UpdateMode((uint32_t)d->id, d->orig_mode, d->orig_mode_data);

                std::lock_guard<std::mutex> sg(s.state_mtx);
                d->engaged     = false;
                d->dirty       = false;
                d->touched     = false;
                d->active_mode = d->orig_mode;
                d->last_sent.clear();
            }
        }

        std::string handleDisconnect(const std::string &reason) {
            auto &s = S();

            s.client.Disconnect();
            s.connected = false;
            s.next_reconnect = Clock::now() + std::chrono::seconds(2);

            {
                std::lock_guard<std::mutex> sg(s.state_mtx);
                for(auto &d : deviceList()) {
                    d->connected = false;
                    d->id        = -1;
                    d->engaged   = false;
                    d->last_sent.clear();
                    if(d->touched)
                        d->dirty = true;
                }
            }

            Logger << "Connection lost: " << reason;
            return "OpenRGB connection lost: " + reason;
        }

        std::string tryReconnect() {
            auto &s = S();
            if(Clock::now() < s.next_reconnect)
                return "";

            if(s.client.Connect(s.host, s.port, 1000) && handshake()) {
                s.connected = true;
                Logger << "Reconnected to OpenRGB server";
                enumerateControllersLocked();
                return "";
            }

            s.client.Disconnect();
            s.next_reconnect = Clock::now() + std::chrono::seconds(3);
            return "";
        }

        /// One worker iteration: compose, diff and flush. Returns an error message if any.
        std::string tick() {
            auto &s = S();
            std::lock_guard<std::mutex> cg(s.client_mtx);

            if(!s.initialized)
                return "";

            if(!s.client.IsConnected())
                return tryReconnect();

            auto ev = s.client.PollEvent(0);
            if(ev < 0)
                return handleDisconnect(s.client.GetLastError());
            else if(ev == (int32_t)LL::NET_PACKET_ID_DEVICE_LIST_UPDATED) {
                Logger << "OpenRGB device list changed, re-enumerating devices...";
                enumerateControllersLocked();
            }

            struct Job {
                std::shared_ptr<DImpl> d;
                std::vector<LL::Color> colors;
                uint64_t version = 0;
                bool engage = false;
                int direct_mode = -1;
                LL::Mode mode;
            };

            std::vector<Job> jobs;
            {
                std::lock_guard<std::mutex> sg(s.state_mtx);

                // Animations will be progressed here using the elapsed time

                for(auto &d : deviceList()) {
                    if(!d->dirty || !d->connected || d->id < 0)
                        continue;

                    if(d->direct_mode < 0) {
                        d->dirty = false;
                        if(!d->unsupported) {
                            d->unsupported = true;
                            Logger << "Device " << d->name << " does not support direct mode, ignoring";
                        }
                        continue;
                    }

                    Job j;
                    j.d           = d;
                    j.colors      = compose(*d);
                    j.version     = d->revision;
                    j.engage      = !d->engaged;
                    j.direct_mode = d->direct_mode;
                    if(j.engage)
                        j.mode = d->modes[d->direct_mode];

                    jobs.push_back(std::move(j));
                }
            }

            for(auto &j : jobs) {
                bool ok = true;

                if(j.engage)
                    ok = s.client.UpdateMode((uint32_t)j.d->id, j.direct_mode, j.mode);
                else if(j.colors == j.d->last_sent) {
                    std::lock_guard<std::mutex> sg(s.state_mtx);
                    if(j.d->revision == j.version)
                        j.d->dirty = false;
                    continue;
                }

                if(ok)
                    ok = s.client.UpdateLEDs((uint32_t)j.d->id, j.colors);

                if(!ok)
                    return handleDisconnect(s.client.GetLastError());

                j.d->last_sent = j.colors;

                std::lock_guard<std::mutex> sg(s.state_mtx);
                if(j.engage) {
                    j.d->engaged     = true;
                    j.d->active_mode = j.direct_mode;
                }
                if(j.d->revision == j.version)
                    j.d->dirty = false;
            }

            return "";
        }

        void workerLoop() {
            auto &s = S();
            Logger << "OpenRGB worker started";

            while(s.running && !Gorgon::exiting) {
                {
                    std::unique_lock<std::mutex> lk(s.wake_mtx);
                    s.wake.wait_for(lk, std::chrono::milliseconds(1000 / std::max(1, s.fps.load())),
                                    []() { return !S().running.load(); });
                }

                if(!s.running || Gorgon::exiting)
                    break;

                fireError(tick());
            }

            Logger << "OpenRGB worker stopped";
        }

        void stopWorker() {
            auto &s = S();
            s.running = false;
            s.wake.notify_all();

            if(s.worker.joinable() && s.worker.get_id() != std::this_thread::get_id())
                s.worker.join();
        }

        void exitfn() {
            Shutdown();
        }

        bool connectNow(const std::string &host, uint16_t port, bool force) {
            auto &s = S();
            std::unique_lock<std::mutex> ig(s.init_mtx);

            if(s.initialized)
                return true;

            if(!force && Clock::now() < s.next_retry)
                return false;

            s.host = host;
            s.port = port;

            std::string err;
            {
                std::lock_guard<std::mutex> cg(s.client_mtx);

                s.client.Disconnect();
                if(!s.client.Connect(host, port, 1000) || !handshake()) {
                    err = s.client.GetLastError();
                    s.client.Disconnect();
                    s.next_retry = Clock::now() + std::chrono::seconds(5);
                }
                else {
                    s.connected = true;
                    enumerateControllersLocked();
                }
            }

            // Events are fired without holding any lock so handlers may call back into the API
            if(!s.connected) {
                ig.unlock();

                Logger << "Cannot connect to OpenRGB server: " << err;
                fireError("Cannot connect to OpenRGB server: " + err);
                return false;
            }

            if(!s.atexit_registered) {
                s.atexit_registered = true;
                std::atexit(&exitfn);
            }

            s.initialized = true;
            s.running     = true;
            s.worker      = std::thread(workerLoop);

            Logger << "OpenRGB initialized with " << s.devices.size() << " device(s)";

            ig.unlock();
            return true;
        }

        bool ensure() {
            auto &s = S();
            if(s.initialized)
                return true;

            return connectNow(s.host, s.port, false);
        }

        /// Applies a hardware mode synchronously
        bool applyMode(const std::shared_ptr<DImpl> &d, int idx) {
            if(!ensure())
                return false;

            auto &s = S();
            std::lock_guard<std::mutex> cg(s.client_mtx);

            if(!s.client.IsConnected())
                return false;

            LL::Mode mode;
            bool wasengaged;
            {
                std::lock_guard<std::mutex> sg(s.state_mtx);
                if(!d->connected || d->id < 0 || idx < 0 || idx >= (int)d->modes.size())
                    return false;

                mode = d->modes[idx];
                wasengaged = d->engaged;
            }

            bool direct = isDirectMode(mode);

            if(!direct && wasengaged)
                s.client.UpdateLEDs((uint32_t)d->id, std::vector<LL::Color>(d->leds.size(), LL::Color(0, 0, 0, 0)));

            if(!s.client.UpdateMode((uint32_t)d->id, idx, mode))
                return false;

            std::lock_guard<std::mutex> sg(s.state_mtx);
            d->active_mode = idx;
            d->engaged     = direct;
            d->touched     = true;
            d->last_sent.clear();
            d->dirty       = direct;
            if(direct)
                d->revision++;

            return true;
        }

        // ---------------------------------------------------------
        // Image helpers
        // ---------------------------------------------------------

        Graphics::RGBA imagePixel(const Containers::Image &img, int x, int y) {
            using Graphics::ColorMode;
            auto c = [&](unsigned ch) { return img(x, y, ch); };

            switch(img.GetMode()) {
            case ColorMode::RGBA:
                return Graphics::RGBA(c(0), c(1), c(2), c(3));
            case ColorMode::RGB:
                return Graphics::RGBA(c(0), c(1), c(2), Byte(255));
            case ColorMode::BGRA:
                return Graphics::RGBA(c(2), c(1), c(0), c(3));
            case ColorMode::BGR:
                return Graphics::RGBA(c(2), c(1), c(0), Byte(255));
            case ColorMode::Grayscale:
                return Graphics::RGBA(c(0), c(0), c(0), Byte(255));
            case ColorMode::Grayscale_Alpha:
                return Graphics::RGBA(c(0), c(0), c(0), c(1));
            case ColorMode::Alpha:
                return Graphics::RGBA(Byte(0), Byte(0), Byte(0), c(0));
            default:
                return transparent;
            }
        }

        Graphics::RGBA blendOver(const Graphics::RGBA &cur, const Graphics::RGBA &px) {
            return lerp(cur, px, px.A / 255.f);
        }

        /// state_mtx must be held
        void drawImage(ZImpl &z, const Containers::Image &img, Geometry::Point offset, BlendMode blend) {
            auto d = z.dev.lock();
            if(!d || z.width == 0 || z.height == 0)
                return;

            int w = img.GetWidth(), h = img.GetHeight();

            for(int y = 0; y < h; y++) {
                int dy = offset.Y + y;
                if(dy < 0 || dy >= z.height)
                    continue;

                for(int x = 0; x < w; x++) {
                    int dx = offset.X + x;
                    if(dx < 0 || dx >= z.width)
                        continue;

                    int local = z.matrix[(size_t)dy * z.width + dx];
                    if(local < 0)
                        continue;

                    int gi = z.led_offset + local;
                    auto px = imagePixel(img, x, y);

                    if(blend == BlendMode::AlphaBlend)
                        setLED(*d, gi, blendOver(getLED(*d, gi), px));
                    else
                        setLED(*d, gi, px);
                }
            }
        }

        void drawScaled(ZImpl &z, const Containers::Image &img, Containers::InterpolationMethod interp, BlendMode blend) {
            if(z.width == 0 || z.height == 0 || img.GetWidth() == 0 || img.GetHeight() == 0)
                return;

            auto scaled = img.Scale({z.width, z.height}, interp);
            drawImage(z, scaled, {0, 0}, blend);
        }

        Containers::Image toImage(const ZImpl &z) {
            auto d = z.dev.lock();
            if(!d || z.width == 0 || z.height == 0)
                return Containers::Image();

            Containers::Image img({z.width, z.height}, Graphics::ColorMode::RGBA);

            for(int y = 0; y < z.height; y++) {
                for(int x = 0; x < z.width; x++) {
                    int local = z.matrix[(size_t)y * z.width + x];

                    Graphics::RGBA c = transparent;
                    if(local >= 0)
                        c = getLED(*d, z.led_offset + local);

                    img(x, y, 0) = c.R;
                    img(x, y, 1) = c.G;
                    img(x, y, 2) = c.B;
                    img(x, y, 3) = local >= 0 ? Byte(255) : Byte(0);
                }
            }

            return img;
        }

    } // anonymous namespace

    // -------------------------------------------------------------
    // KeyGroup
    // -------------------------------------------------------------

    namespace {
        KeyGroup numbers() {
            return {{K::Number_1, K::Number_2, K::Number_3, K::Number_4, K::Number_5, K::Number_6, K::Number_7, K::Number_8, K::Number_9, K::Number_0}};
        }

        KeyGroup functionKeys() {
            return {{K::F1, K::F2, K::F3, K::F4, K::F5, K::F6, K::F7, K::F8, K::F9, K::F10, K::F11, K::F12}};
        }

        KeyGroup numpadDigits() {
            return {{K::Numpad_1, K::Numpad_2, K::Numpad_3, K::Numpad_4, K::Numpad_5, K::Numpad_6, K::Numpad_7, K::Numpad_8, K::Numpad_9, K::Numpad_0}};
        }

        KeyGroup letters() {
            KeyGroup g;
            for(char c = 'A'; c <= 'Z'; c++)
                g.keys.push_back(c);

            return g;
        }

        KeyGroup everything() {
            return {AllKnownKeys()};
        }
    }

    const KeyGroup KeyGroup::WASD         = {{K::W, K::A, K::S, K::D}};
    const KeyGroup KeyGroup::Arrows       = {{K::Up, K::Left, K::Down, K::Right}};
    const KeyGroup KeyGroup::NumberRow    = numbers();
    const KeyGroup KeyGroup::FunctionKeys = functionKeys();
    const KeyGroup KeyGroup::Modifiers    = {{K::Shift, K::Control, K::Alt, K::Meta, K::RShift, K::RControl, K::RAlt, K::RMeta}};
    const KeyGroup KeyGroup::NumpadDigits = numpadDigits();
    const KeyGroup KeyGroup::Numpad       = {{
        K::Numlock, K::Numpad_Div, K::Numpad_Mult, K::Numpad_Minus,
        K::Numpad_7, K::Numpad_8, K::Numpad_9, K::Numpad_Plus,
        K::Numpad_4, K::Numpad_5, K::Numpad_6,
        K::Numpad_1, K::Numpad_2, K::Numpad_3, K::Numpad_Enter,
        K::Numpad_0, K::Numpad_Decimal
    }};
    const KeyGroup KeyGroup::Letters      = letters();
    const KeyGroup KeyGroup::All          = everything();

    const KeyGroup KeyGroup::ProgressBarTargets::FunctionKeys = functionKeys();
    const KeyGroup KeyGroup::ProgressBarTargets::NumberRow    = numbers();
    const KeyGroup KeyGroup::ProgressBarTargets::NumpadDigits = numpadDigits();

    // -------------------------------------------------------------
    // Zone
    // -------------------------------------------------------------

    Zone::Zone() : impl(std::make_shared<Impl>()) { }

    Zone::Zone(int index, const std::string &name, ZoneType type, int led_count, int led_offset, const Device &parent) :
        impl(std::make_shared<Impl>())
    {
        impl->index      = index;
        impl->name       = name;
        impl->type       = type;
        impl->led_count  = led_count;
        impl->led_offset = led_offset;
        impl->dev        = Access::impl(parent);
    }

    int Zone::GetIndex() const { return impl->index; }

    const std::string &Zone::GetName() const { return impl->name; }

    ZoneType Zone::GetType() const { return impl->type; }

    int Zone::GetLEDCount() const { return impl->led_count; }

    int Zone::GetLEDOffset() const { return impl->led_offset; }

    bool Zone::IsConnected() const {
        auto d = impl->dev.lock();
        return d && d->connected;
    }

    std::optional<Device> Zone::GetDevice() const {
        auto d = impl->dev.lock();
        if(!d || !d->connected)
            return std::nullopt;

        return Device(d);
    }

    Graphics::RGBA Zone::GetLEDColor(int local_idx) const {
        auto d = impl->dev.lock();
        if(!d || local_idx < 0 || local_idx >= impl->led_count)
            return transparent;

        std::lock_guard<std::mutex> g(S().state_mtx);
        return getLED(*d, impl->led_offset + local_idx);
    }

    void Zone::SetLEDColor(int local_idx, Graphics::RGBA color) {
        auto d = impl->dev.lock();
        if(!d || local_idx < 0 || local_idx >= impl->led_count)
            return;

        std::lock_guard<std::mutex> g(S().state_mtx);
        setLED(*d, impl->led_offset + local_idx, color);
    }

    void Zone::SetColor(Graphics::RGBA color) {
        auto d = impl->dev.lock();
        if(!d)
            return;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(int i = 0; i < impl->led_count; i++)
            setLED(*d, impl->led_offset + i, color);
    }

    void Zone::Clear() {
        SetColor(Graphics::RGBA(Byte(0), Byte(0), Byte(0), Byte(255)));
    }

    bool Zone::IsMatrix() const { return impl->width > 0 && impl->height > 0; }

    Geometry::Size Zone::GetMatrixSize() const { return {impl->width, impl->height}; }

    int Zone::GetWidth() const { return impl->width; }

    int Zone::GetHeight() const { return impl->height; }

    int Zone::GetLEDIndexAt(int x, int y) const {
        if(x < 0 || y < 0 || x >= impl->width || y >= impl->height)
            return -1;

        return impl->matrix[(size_t)y * impl->width + x];
    }

    int Zone::GetLEDIndexAt(Geometry::Point p) const { return GetLEDIndexAt(p.X, p.Y); }

    Graphics::RGBA Zone::GetPixel(int x, int y) const {
        int idx = GetLEDIndexAt(x, y);
        if(idx < 0)
            return transparent;

        return GetLEDColor(idx);
    }

    Graphics::RGBA Zone::GetPixel(Geometry::Point p) const { return GetPixel(p.X, p.Y); }

    void Zone::SetPixel(int x, int y, Graphics::RGBA color) {
        int idx = GetLEDIndexAt(x, y);
        if(idx >= 0)
            SetLEDColor(idx, color);
    }

    void Zone::SetPixel(Geometry::Point p, Graphics::RGBA color) { SetPixel(p.X, p.Y, color); }

    void Zone::Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset, BlendMode blend) {
        if(!bitmap.HasData())
            return;

        Draw(bitmap.GetData(), offset, blend);
    }

    void Zone::Draw(const Containers::Image &image, Geometry::Point offset, BlendMode blend) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        drawImage(*impl, image, offset, blend);
    }

    void Zone::DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp, BlendMode blend) {
        if(!bitmap.HasData())
            return;

        DrawScaled(bitmap.GetData(), interp, blend);
    }

    void Zone::DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp, BlendMode blend) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        drawScaled(*impl, image, interp, blend);
    }

    Containers::Image Zone::ToImage() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        return toImage(*impl);
    }

    // -------------------------------------------------------------
    // Device
    // -------------------------------------------------------------

    Device::Device() : impl(std::make_shared<Impl>()) { }

    Device::Device(std::shared_ptr<Impl> impl) : impl(std::move(impl)) { }

    int Device::GetID() const { return impl->id; }

    const std::string &Device::GetName() const { return impl->name; }

    const std::string &Device::GetVendor() const { return impl->vendor; }

    const std::string &Device::GetDescription() const { return impl->description; }

    const std::string &Device::GetVersion() const { return impl->version; }

    const std::string &Device::GetSerial() const { return impl->serial; }

    const std::string &Device::GetLocation() const { return impl->location; }

    DeviceType Device::GetType() const { return impl->type; }

    bool Device::IsConnected() const { return impl->connected; }

    int Device::GetLEDCount() const { return (int)impl->leds.size(); }

    const std::vector<LED> &Device::GetLEDs() const { return impl->leds; }

    int Device::FindLED(const std::string &name) const {
        auto target = normalize(name);

        for(size_t i = 0; i < impl->leds.size(); i++) {
            if(normalize(impl->leds[i].name) == target)
                return (int)i;
        }

        return -1;
    }

    Graphics::RGBA Device::GetLEDColor(int led_idx) const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        return getLED(*impl, led_idx);
    }

    Graphics::RGBA Device::GetLEDColor(const std::string &name) const {
        return GetLEDColor(FindLED(name));
    }

    void Device::SetLEDColor(int led_idx, Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        setLED(*impl, led_idx, color);
    }

    void Device::SetLEDColor(const std::string &name, Graphics::RGBA color) {
        SetLEDColor(FindLED(name), color);
    }

    void Device::SetColor(Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        for(int i = 0; i < (int)impl->leds.size(); i++)
            setLED(*impl, i, color);
    }

    void Device::SetColors(const std::vector<Graphics::RGBA> &colors) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        for(int i = 0; i < (int)impl->leds.size() && i < (int)colors.size(); i++)
            setLED(*impl, i, colors[i]);
    }

    void Device::Clear() {
        SetColor(Graphics::RGBA(Byte(0), Byte(0), Byte(0), Byte(255)));
    }

    int Device::GetZoneCount() const { return (int)impl->zones.size(); }

    Zone &Device::GetZone(int index) { return impl->zones.at(index); }

    const Zone &Device::GetZone(int index) const { return impl->zones.at(index); }

    bool Device::HasZone(const std::string &name) const {
        auto n = lower(name);
        for(auto &z : impl->zones) {
            if(lower(z.GetName()) == n)
                return true;
        }

        return false;
    }

    std::optional<Zone> Device::FindZone(const std::string &name) const {
        auto n = lower(name);
        for(auto &z : impl->zones) {
            if(lower(z.GetName()) == n)
                return z;
        }

        for(auto &z : impl->zones) {
            if(lower(z.GetName()).find(n) != std::string::npos)
                return z;
        }

        return std::nullopt;
    }

    std::vector<Zone> Device::GetZones() const { return impl->zones; }

    int Device::GetModeCount() const { return (int)impl->modes.size(); }

    Device::Mode Device::GetActiveMode() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        return impl->active_mode;
    }

    std::string Device::GetActiveModeName() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        if(impl->active_mode < 0 || impl->active_mode >= (int)impl->modes.size())
            return "";

        return impl->modes[impl->active_mode].name;
    }

    void Device::SetMode(Mode mode_index) {
        applyMode(impl, mode_index);
    }

    void Device::SetMode(const std::string &mode_name) {
        auto n = lower(mode_name);
        for(int i = 0; i < (int)impl->modes.size(); i++) {
            if(lower(impl->modes[i].name) == n) {
                applyMode(impl, i);
                return;
            }
        }
    }

    bool Device::SetDirectMode() {
        if(impl->direct_mode < 0)
            return false;

        return applyMode(impl, impl->direct_mode);
    }

    // -------------------------------------------------------------
    // KeyboardDevice
    // -------------------------------------------------------------

    KeyboardDevice::KeyboardDevice(std::shared_ptr<Impl> impl) : Device(std::move(impl)) { }

    void KeyboardDevice::SetKeyColor(Input::Key key, Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);

        auto it = impl->keymap.find(key);
        if(it == impl->keymap.end())
            return;

        for(int led : it->second)
            setLED(*impl, led, color);
    }

    void KeyboardDevice::SetKeyColor(const std::string &key_name, Graphics::RGBA color) {
        SetLEDColor(FindLED(key_name), color);
    }

    Graphics::RGBA KeyboardDevice::GetKeyColor(Input::Key key) const {
        std::lock_guard<std::mutex> g(S().state_mtx);

        auto it = impl->keymap.find(key);
        if(it == impl->keymap.end() || it->second.empty())
            return transparent;

        return getLED(*impl, it->second[0]);
    }

    void KeyboardDevice::SetKeyGroupColor(const KeyGroup &group, Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);

        for(auto key : group.keys) {
            auto it = impl->keymap.find(key);
            if(it == impl->keymap.end())
                continue;

            for(int led : it->second)
                setLED(*impl, led, color);
        }
    }

    bool KeyboardDevice::HasKey(Input::Key key) const {
        return impl->keymap.count(key) != 0;
    }

    std::vector<int> KeyboardDevice::GetKeyLEDIndices(Input::Key key) const {
        auto it = impl->keymap.find(key);
        if(it == impl->keymap.end())
            return {};

        return it->second;
    }

    std::optional<Zone> KeyboardDevice::GetMatrixZone() const {
        if(impl->matrix_zone < 0 || impl->matrix_zone >= (int)impl->zones.size())
            return std::nullopt;

        return impl->zones[impl->matrix_zone];
    }

    Geometry::Size KeyboardDevice::GetMatrixSize() const {
        auto z = GetMatrixZone();
        return z ? z->GetMatrixSize() : Geometry::Size(0, 0);
    }

    int KeyboardDevice::GetWidth() const {
        auto z = GetMatrixZone();
        return z ? z->GetWidth() : 0;
    }

    int KeyboardDevice::GetHeight() const {
        auto z = GetMatrixZone();
        return z ? z->GetHeight() : 0;
    }

    Graphics::RGBA KeyboardDevice::GetPixel(int x, int y) const {
        auto z = GetMatrixZone();
        return z ? z->GetPixel(x, y) : transparent;
    }

    Graphics::RGBA KeyboardDevice::GetPixel(Geometry::Point p) const {
        return GetPixel(p.X, p.Y);
    }

    void KeyboardDevice::SetPixel(int x, int y, Graphics::RGBA color) {
        if(auto z = GetMatrixZone())
            z->SetPixel(x, y, color);
    }

    void KeyboardDevice::SetPixel(Geometry::Point p, Graphics::RGBA color) {
        SetPixel(p.X, p.Y, color);
    }

    void KeyboardDevice::Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset, BlendMode blend) {
        if(auto z = GetMatrixZone())
            z->Draw(bitmap, offset, blend);
    }

    void KeyboardDevice::Draw(const Containers::Image &image, Geometry::Point offset, BlendMode blend) {
        if(auto z = GetMatrixZone())
            z->Draw(image, offset, blend);
    }

    void KeyboardDevice::DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp, BlendMode blend) {
        if(auto z = GetMatrixZone())
            z->DrawScaled(bitmap, interp, blend);
    }

    void KeyboardDevice::DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp, BlendMode blend) {
        if(auto z = GetMatrixZone())
            z->DrawScaled(image, interp, blend);
    }

    Containers::Image KeyboardDevice::ToImage() const {
        auto z = GetMatrixZone();
        return z ? z->ToImage() : Containers::Image();
    }

    // -------------------------------------------------------------
    // Progressbar
    // -------------------------------------------------------------

    namespace {

        /// An ordered list of positions (each position may light multiple LEDs, e.g. spacebar)
        struct Strip {
            std::shared_ptr<DImpl> dev;
            std::vector<std::vector<int>> positions;
        };

        Strip keyStrip(const std::shared_ptr<DImpl> &d, const KeyGroup &keys, int begin, int end) {
            Strip s;
            s.dev = d;

            for(auto key : keys.keys) {
                auto it = d->keymap.find(key);
                if(it == d->keymap.end())
                    continue;

                std::vector<int> leds;
                for(int l : it->second) {
                    if(l >= begin && l < end)
                        leds.push_back(l);
                }

                if(!leds.empty())
                    s.positions.push_back(leds);
            }

            return s;
        }

        /// state_mtx must be held
        std::vector<Strip> resolve(const PImpl &p) {
            std::vector<Strip> strips;

            switch(p.target) {
            case PImpl::Target::AllKeyboards:
                for(auto &d : deviceList()) {
                    if(d->is_keyboard && d->connected) {
                        auto s = keyStrip(d, p.keys, 0, (int)d->leds.size());
                        if(!s.positions.empty())
                            strips.push_back(std::move(s));
                    }
                }
                break;

            case PImpl::Target::Keyboard:
                if(auto d = p.device.lock()) {
                    if(d->connected) {
                        auto s = keyStrip(d, p.keys, 0, (int)d->leds.size());
                        if(!s.positions.empty())
                            strips.push_back(std::move(s));
                    }
                }
                break;

            case PImpl::Target::Zone:
                if(p.zone) {
                    if(auto d = p.zone->dev.lock()) {
                        if(d->connected) {
                            int begin = p.zone->led_offset, end = begin + p.zone->led_count;

                            if(p.use_keys) {
                                auto s = keyStrip(d, p.keys, begin, end);
                                if(!s.positions.empty())
                                    strips.push_back(std::move(s));
                            }
                            else {
                                Strip s;
                                s.dev = d;
                                for(int i = begin; i < end; i++)
                                    s.positions.push_back({i});

                                if(!s.positions.empty())
                                    strips.push_back(std::move(s));
                            }
                        }
                    }
                }
                break;

            default:
                break;
            }

            return strips;
        }

        Graphics::RGBA levelColor(const PImpl &p) {
            for(auto &l : p.levels) {
                bool inside = p.ratio >= l.range.Start &&
                              (p.ratio < l.range.End || (l.range.End >= 1.f && p.ratio <= l.range.End));

                if(inside)
                    return l.color;
            }

            return p.color;
        }

        /// state_mtx must be held
        void applyProgress(PImpl &p) {
            auto strips = resolve(p);
            auto fg = levelColor(p);

            for(auto &s : strips) {
                int n = (int)s.positions.size();

                for(int i = 0; i < n; i++) {
                    float t;
                    if(p.fill) {
                        t = Clamp(p.ratio * n - i, 0.f, 1.f);
                    }
                    else if(n == 1) {
                        t = p.ratio;
                    }
                    else {
                        float pos = p.ratio * (n - 1);
                        t = Clamp(1.f - std::abs(pos - i), 0.f, 1.f);
                    }

                    for(int led : s.positions[i]) {
                        auto key = std::make_pair(s.dev, led);

                        auto it = p.underlay.find(key);
                        if(it == p.underlay.end())
                            it = p.underlay.emplace(key, getLED(*s.dev, led)).first;

                        Graphics::RGBA bg = it->second;
                        if(p.bg.A != 0)
                            bg = lerp(it->second, p.bg, p.bg.A / 255.f);

                        setLED(*s.dev, led, lerp(bg, fg, t));
                    }
                }
            }

            p.active = true;
        }

        void refresh(PImpl &p) {
            std::lock_guard<std::mutex> g(S().state_mtx);
            if(p.active)
                applyProgress(p);
        }
    }

    Progressbar::Progressbar() : impl(std::make_shared<Impl>()) { }

    Progressbar::Progressbar(const KeyGroup &keys, Graphics::RGBA color, Graphics::RGBA bg) : Progressbar() {
        impl->target   = Impl::Target::AllKeyboards;
        impl->keys     = keys;
        impl->use_keys = true;
        impl->color    = color;
        impl->bg       = bg;
    }

    Progressbar::Progressbar(const KeyGroup &keys, const KeyboardDevice &keyboard, Graphics::RGBA color, Graphics::RGBA bg) : Progressbar() {
        impl->target   = Impl::Target::Keyboard;
        impl->device   = Access::impl(keyboard);
        impl->keys     = keys;
        impl->use_keys = true;
        impl->color    = color;
        impl->bg       = bg;
    }

    Progressbar::Progressbar(const KeyGroup &keys, const Zone &zone, Graphics::RGBA color, Graphics::RGBA bg) : Progressbar() {
        impl->target   = Impl::Target::Zone;
        impl->zone     = Access::impl(zone);
        impl->keys     = keys;
        impl->use_keys = true;
        impl->color    = color;
        impl->bg       = bg;
    }

    Progressbar::Progressbar(const Zone &zone, Graphics::RGBA color, Graphics::RGBA bg) : Progressbar() {
        impl->target   = Impl::Target::Zone;
        impl->zone     = Access::impl(zone);
        impl->use_keys = false;
        impl->color    = color;
        impl->bg       = bg;
    }

    void Progressbar::SetKeys(const KeyGroup &keys) {
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            impl->keys     = keys;
            impl->use_keys = true;
        }
        refresh(*impl);
    }

    const KeyGroup &Progressbar::GetKeys() const { return impl->keys; }

    void Progressbar::SetZone(const Zone &zone) {
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            impl->target = Impl::Target::Zone;
            impl->zone   = Access::impl(zone);
        }
        refresh(*impl);
    }

    void Progressbar::SetColor(Graphics::RGBA color) {
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            impl->color = color;
        }
        refresh(*impl);
    }

    Graphics::RGBA Progressbar::GetColor() const { return impl->color; }

    void Progressbar::SetBackgroundColor(Graphics::RGBA bg) {
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            impl->bg = bg;
        }
        refresh(*impl);
    }

    Graphics::RGBA Progressbar::GetBackgroundColor() const { return impl->bg; }

    void Progressbar::AddLevelColor(Range<Float> range, Graphics::RGBA color) {
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            impl->levels.push_back({range, color});
        }
        refresh(*impl);
    }

    void Progressbar::SetLevelColors(const std::vector<LevelColor> &levels) {
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            impl->levels = levels;
        }
        refresh(*impl);
    }

    void Progressbar::ClearLevelColors() {
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            impl->levels.clear();
        }
        refresh(*impl);
    }

    const std::vector<Progressbar::LevelColor> &Progressbar::GetLevelColors() const { return impl->levels; }

    void Progressbar::SetFillMode(bool fill) {
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            impl->fill = fill;
        }
        refresh(*impl);
    }

    bool Progressbar::IsFillMode() const { return impl->fill; }

    void Progressbar::SetProgressRatio(Float value) {
        if(!ensure())
            return;

        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->ratio = Clamp(value, Float(0), Float(1));
        applyProgress(*impl);
    }

    Float Progressbar::GetProgressRatio() const { return impl->ratio; }

    void Progressbar::Clear() {
        std::lock_guard<std::mutex> g(S().state_mtx);

        for(auto &p : impl->underlay)
            setLED(*p.first.first, p.first.second, p.second);

        impl->underlay.clear();
        impl->active = false;
    }

    // -------------------------------------------------------------
    // Global functions
    // -------------------------------------------------------------

    bool Initialize(const std::string &host, uint16_t port) {
        return connectNow(host, port, true);
    }

    bool IsAvailable() {
        return ensure() && S().connected;
    }

    void Shutdown() {
        auto &s = S();
        std::lock_guard<std::mutex> ig(s.init_mtx);

        if(!s.initialized)
            return;

        stopWorker();

        {
            std::lock_guard<std::mutex> cg(s.client_mtx);
            restoreLocked();
            s.client.Disconnect();
            s.connected = false;
        }

        {
            std::lock_guard<std::mutex> sg(s.state_mtx);
            s.devices.clear();
        }

        s.initialized = false;
        Logger << "OpenRGB shut down";
    }

    void RestoreAll() {
        if(!S().initialized)
            return;

        std::lock_guard<std::mutex> cg(S().client_mtx);
        restoreLocked();
    }

    void Flush() {
        if(!ensure())
            return;

        fireError(tick());
    }

    void SetTargetFPS(int fps) {
        S().fps = Clamp(fps, 10, 120);
        S().wake.notify_all();
    }

    int GetTargetFPS() {
        return S().fps;
    }

    void Refresh() {
        if(!ensure())
            return;

        auto &s = S();
        std::lock_guard<std::mutex> cg(s.client_mtx);
        if(!s.client.IsConnected())
            return;

        s.client.RescanDevices();
        enumerateControllersLocked();
    }

    std::vector<Device> GetDevices(bool only_connected) {
        std::vector<Device> list;
        if(!ensure())
            return list;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(!only_connected || d->IsConnected())
                list.push_back(*d);
        }

        return list;
    }

    std::vector<KeyboardDevice> GetKeyboards(bool only_connected) {
        std::vector<KeyboardDevice> list;
        if(!ensure())
            return list;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(auto kb = dynamic_cast<KeyboardDevice *>(d.get())) {
                if(!only_connected || kb->IsConnected())
                    list.push_back(*kb);
            }
        }

        return list;
    }

    std::optional<Device> FindDevice(const std::string &name) {
        if(!ensure())
            return std::nullopt;

        auto n = lower(name);

        std::lock_guard<std::mutex> g(S().state_mtx);
        // Prefer connected devices
        for(auto &d : S().devices) {
            if(d->IsConnected() && lower(d->GetName()) == n)
                return *d;
        }

        for(auto &d : S().devices) {
            if(d->IsConnected() && lower(d->GetName()).find(n) != std::string::npos)
                return *d;
        }

        // Fallback to detached devices
        for(auto &d : S().devices) {
            if(lower(d->GetName()) == n)
                return *d;
        }

        for(auto &d : S().devices) {
            if(lower(d->GetName()).find(n) != std::string::npos)
                return *d;
        }

        return std::nullopt;
    }

    std::optional<KeyboardDevice> GetPrimaryKeyboard() {
        if(!ensure())
            return std::nullopt;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(d->IsConnected()) {
                if(auto kb = dynamic_cast<KeyboardDevice *>(d.get()))
                    return *kb;
            }
        }

        for(auto &d : S().devices) {
            if(auto kb = dynamic_cast<KeyboardDevice *>(d.get()))
                return *kb;
        }

        return std::nullopt;
    }

    void SetKeyColor(Input::Key key, Graphics::RGBA color) {
        for(auto &kb : GetKeyboards())
            kb.SetKeyColor(key, color);
    }

    void SetKeyGroupColor(const KeyGroup &group, Graphics::RGBA color) {
        for(auto &kb : GetKeyboards())
            kb.SetKeyGroupColor(group, color);
    }

    void Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset, BlendMode blend) {
        for(auto &d : GetDevices()) {
            for(auto &z : d.GetZones()) {
                if(z.IsMatrix())
                    z.Draw(bitmap, offset, blend);
            }
        }
    }

    void Draw(const Containers::Image &image, Geometry::Point offset, BlendMode blend) {
        for(auto &d : GetDevices()) {
            for(auto &z : d.GetZones()) {
                if(z.IsMatrix())
                    z.Draw(image, offset, blend);
            }
        }
    }

    void DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp, BlendMode blend) {
        for(auto &d : GetDevices()) {
            for(auto &z : d.GetZones()) {
                if(z.IsMatrix())
                    z.DrawScaled(bitmap, interp, blend);
            }
        }
    }

    void DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp, BlendMode blend) {
        for(auto &d : GetDevices()) {
            for(auto &z : d.GetZones()) {
                if(z.IsMatrix())
                    z.DrawScaled(image, interp, blend);
            }
        }
    }

    void SetAllColor(Graphics::RGBA color) {
        for(auto &d : GetDevices())
            d.SetColor(color);
    }

    void ClearAll() {
        for(auto &d : GetDevices())
            d.Clear();
    }

} // namespace Gorgon::Devices::OpenRGB
