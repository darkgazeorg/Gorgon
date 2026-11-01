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
#include "Gorgon/String.h"
#include "OpenRGBClient.h"

#include "../../Graphics/Bitmap.h"
#include "../../Input/Keyboard.h"
#include "../../Window.h"
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

    struct DeviceLayer {
        bool touched = false;
        bool mode_dirty = false;
        int active_mode = 0;
        std::vector<LL::Mode> modes;
        std::vector<Graphics::RGBA> base_leds;
    };

    struct Device::Impl {
        int id = -1;
        bool connected = false;
        std::string name, vendor, description, version, serial, location;
        DeviceType type = DeviceType::Unknown;

        std::vector<LED>  leds;      ///< Public LED metadata (`color` mirrors the global layer's base color)
        std::vector<Zone> zones;

        std::vector<LL::Mode> default_modes;
        int direct_mode = -1;

        // Snapshot taken on discovery, used for restoration
        int                   orig_mode = 0;
        LL::Mode              orig_mode_data;
        std::vector<LL::Color> orig_colors;
        bool                  orig_direct = false;
        std::vector<Graphics::RGBA> initial_base_leds;

        // Keyboard info
        bool is_keyboard = false;
        int  matrix_zone = -1;
        std::unordered_map<int, std::vector<int>> keymap;
        std::unordered_map<int, Input::Key> led_to_key;

        // Per-context layers (state_mtx)
        DeviceLayer global_layer;
        std::unordered_map<const Gorgon::Animation::Governor *, DeviceLayer> gov_layers;

        // Runtime hardware tracking (state_mtx)
        bool     hw_touched  = false; ///< Physical hardware has been modified by us since connection
        bool     engaged     = false; ///< Physical hardware is currently in Direct mode driven by us
        int      hw_mode     = -1;    ///< Last mode index sent to physical hardware
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
        using AImpl = Animation::Impl;

        static std::shared_ptr<DImpl> impl(const Device &d)      { return d.impl; }
        static std::shared_ptr<ZImpl> impl(const Zone &z)        { return z.impl; }
        static std::shared_ptr<PImpl> impl(const Progressbar &p) { return p.impl; }
        static std::shared_ptr<AImpl> impl(const Animation &a)   { return a.impl; }

        static const Gorgon::Animation::Governor *context(const Device &d) { return d.context; }
        static const Gorgon::Animation::Governor *context(const Zone &z)   { return z.context; }

        static void setImpl(Zone &z, std::shared_ptr<ZImpl> i, const Gorgon::Animation::Governor *ctx = nullptr) {
            z.impl = std::move(i);
            z.context = ctx;
        }

        static Zone makeZone(const Zone &z, const Gorgon::Animation::Governor *ctx) {
            Zone copy = z;
            copy.context = ctx;
            return copy;
        }

        static Device makeDevice(std::shared_ptr<DImpl> i, const Gorgon::Animation::Governor *ctx = nullptr) {
            return Device(std::move(i), ctx);
        }

        static KeyboardDevice makeKeyboard(std::shared_ptr<DImpl> i, const Gorgon::Animation::Governor *ctx = nullptr) {
            return KeyboardDevice(std::move(i), ctx);
        }

        static Device         *newDevice(std::shared_ptr<DImpl> i)   { return new Device(std::move(i), nullptr); }
        static KeyboardDevice *newKeyboard(std::shared_ptr<DImpl> i) { return new KeyboardDevice(std::move(i), nullptr); }
    };

    struct Progressbar::Impl {
        enum class Target { None, AllKeyboards, AllStrips, Keyboard, Device, Zone };

        const Gorgon::Animation::Governor *context = nullptr;
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
    };

    struct Animation::Impl {
        enum class Type { Base, Fade, Pulse, Blink, Explosion, Swipe, Reactive };
        enum class Target { AllDevices, AllKeyboards, AllStrips, Device, Keyboard, Zone };
        enum class OriginMode { Normalized, Point, Key };

        Type type = Type::Base;
        Target target = Target::AllDevices;
        const Gorgon::Animation::Governor *context = nullptr;

        std::weak_ptr<Access::DImpl> device;
        std::shared_ptr<Access::ZImpl> zone;
        KeyGroup keys;
        bool use_keys = false;

        unsigned duration_ms = 1000;
        BlendMode blend = BlendMode::AlphaBlend;
        bool keep_when_finished = false;
        bool visible = false;
        unsigned current_progress_ms = 0;

        std::unique_ptr<Gorgon::Animation::Controller> owned_ctrl;

        // Fade / Pulse / Blink / Explosion / Swipe parameters
        std::vector<Graphics::RGBA> stops = {Graphics::RGBA(0xffff0000u), Graphics::RGBA(0x00000000u)};
        float duty_cycle = 0.5f;

        // Explosion parameters
        OriginMode origin_mode = OriginMode::Normalized;
        Geometry::Pointf origin_norm = {0.5f, 0.5f};
        Geometry::Point origin_pt = {0, 0};
        Input::Key origin_key = 0;
        float ring_thickness = 3.5f;
        float max_radius = 0.f;

        // Swipe parameters
        SwipeDirection swipe_dir = SwipeDirection::LeftToRight;
        float band_width = 0.25f;

        // Reactive parameters
        struct KeyPressState {
            Input::Key key = 0;
            unsigned start_ms = 0;
            Graphics::RGBA color;
        };
        std::vector<KeyPressState> active_presses;
        bool random_reactive = false;
        float ripple_radius = 0.f;
        unsigned reactive_clock_ms = 0;
        Gorgon::Window *attached_window = nullptr;
        EventToken window_key_token = 0;
        uint32_t rng_state = 0x12345678u;

        ~Impl() {
            if(attached_window && window_key_token) {
                attached_window->KeyPreviewEvent.Unregister(window_key_token);
                attached_window = nullptr;
                window_key_token = 0;
            }
        }
    };

    using DImpl = Access::DImpl;
    using ZImpl = Access::ZImpl;
    using PImpl = Access::PImpl;
    using AImpl = Access::AImpl;

    namespace {

        using Clock = std::chrono::steady_clock;

        struct State {
            std::mutex state_mtx, client_mtx, init_mtx, wake_mtx;
            std::condition_variable wake;

            LL::Client client;
            std::vector<std::unique_ptr<Device>> devices; ///< Registry; stable addresses until Shutdown

            std::vector<std::weak_ptr<PImpl>> progressbars;
            std::vector<std::weak_ptr<AImpl>> animations;
            std::vector<std::weak_ptr<internal::ShortcutOverlay>> shortcut_overlays;
            std::map<const Gorgon::Animation::Governor *, std::shared_ptr<internal::ShortcutOverlay>> context_action_keys;

            std::thread worker;
            std::atomic<bool> running{false};
            std::atomic<bool> initialized{false};
            std::atomic<bool> connected{false};
            std::atomic<int>  fps{30};
            std::atomic<bool> suspended{false};

            std::string host = "127.0.0.1";
            uint16_t    port = 6742;

            Clock::time_point next_retry{};      ///< Lazy initialization cooldown
            Clock::time_point next_reconnect{};  ///< Worker reconnection cooldown
            bool atexit_registered = false;
            bool hooks_registered = false;
        };

        State &S() {
            static State s;
            return s;
        }

        // ---------------------------------------------------------
        // Utilities
        // ---------------------------------------------------------

        /// Lowercases, removes the "Key:" prefix and whitespace for tolerant name matching
        std::string normalize(const std::string &name) {
            std::string s = String::ToLower(name);
            if(s.rfind("key:", 0) == 0)
                s = s.substr(4);

            std::string r;
            std::copy_if(s.begin(), s.end(), std::back_inserter(r), [](char c) { return c != ' ' && c != '_' && c != '\t'; });

            return r;
        }

        LL::Color wire(const Graphics::RGBA &c) {
            return LL::Color(c.R, c.G, c.B, 0);
        }

        Graphics::RGBA opaque(const Graphics::RGBA &c) {
            return Graphics::RGBA(c.R, c.G, c.B, Byte(255));
        }

        //TODO: Use builtin or move to builtin Color subsystem {
        Graphics::RGBA lerp(const Graphics::RGBA &a, const Graphics::RGBA &b, float t) {
            t = Clamp(t, 0.f, 1.f);
            auto mix = [&](Byte x, Byte y) { return Byte(x + (y - x) * t + 0.5f); };

            return Graphics::RGBA(mix(a.R, b.R), mix(a.G, b.G), mix(a.B, b.B), Byte(255));
        }

        Graphics::RGBA lerpRGBA(const Graphics::RGBA &a, const Graphics::RGBA &b, float t) {
            t = Clamp(t, 0.f, 1.f);
            auto mix = [&](Byte x, Byte y) { return Byte(x + (y - x) * t + 0.5f); };

            return Graphics::RGBA(mix(a.R, b.R), mix(a.G, b.G), mix(a.B, b.B), mix(a.A, b.A));
        }

        Graphics::RGBA blendColor(const Graphics::RGBA &cur, const Graphics::RGBA &src, BlendMode mode) {
            float a = src.A / 255.f;
            switch(mode) {
            case BlendMode::Overwrite:
                return opaque(src);
            case BlendMode::AlphaBlend:
                return lerp(cur, src, a);
            case BlendMode::Additive: {
                auto add = [&](Byte c, Byte s) {
                    int v = (int)c + (int)std::round(s * a);
                    return Byte(std::min(255, std::max(0, v)));
                };
                return Graphics::RGBA(add(cur.R, src.R), add(cur.G, src.G), add(cur.B, src.B), Byte(255));
            }
            case BlendMode::Multiply: {
                auto mul = [&](Byte c, Byte s) {
                    float factor = 1.f - a + a * (s / 255.f);
                    int v = (int)std::round(c * factor);
                    return Byte(std::min(255, std::max(0, v)));
                };
                return Graphics::RGBA(mul(cur.R, src.R), mul(cur.G, src.G), mul(cur.B, src.B), Byte(255));
            }
            default:
                return lerp(cur, src, a);
            }
        }

        const Graphics::RGBA transparent = Graphics::RGBA(Byte(0), Byte(0), Byte(0), Byte(0));

        //}

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
                m[K::Enter]     = {"enter", "enter(iso)", "isoenter", "ansienter", "return"};
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

                // Hardware scancode / OSTransport mappings for international & punctuation keys.
                // OpenRGB names physical LEDs after US-ANSI / UK-ISO keycaps (`[`, `]`, `;`, `'`, `#`, `\ (ISO)`, etc.),
                // whereas Gorgon's input system reports `OSTransport + hardware_scancode` for layout-specific keys
                // (e.g., Turkish Ğ, Ü, Ş, İ, Ö, Ç, *, -, ", <, and dotless ı).
                auto add_scancode = [&](int linux_x11_kc, int win_scancode, std::vector<std::string> leds) {
#ifdef WIN32
                    int sc = win_scancode;
#else
                    int sc = linux_x11_kc;
#endif
                    m[K::OSTransport + sc] = std::move(leds);
                };

                // Top row left of '1' (` / " / OEM_3)
                add_scancode(49, 0x29, {"`", "backtick", "grave", "tilde", "~", "section"});
                // Top row right of '0' (- / * / OEM_MINUS)
                add_scancode(20, 0x0C, {"-", "minus", "hyphen", "oemminus"});
                // Top row left of Backspace (= / - / OEM_PLUS)
                add_scancode(21, 0x0D, {"=", "equals", "equal", "plus", "oemplus"});
                // QWERTY row 'I' physical position (on Turkish layouts unshifted 'ı' sends OSTransport + 31 / 0x17)
                add_scancode(31, 0x17, {"i"});
                // QWERTY row right of 'P' ([ / Ğ / OEM_4)
                add_scancode(34, 0x1A, {"[", "leftbracket", "openbracket"});
                // QWERTY row second right of 'P' (] / Ü / OEM_6)
                add_scancode(35, 0x1B, {"]", "rightbracket", "closebracket"});
                // Home row right of 'L' (; / Ş / OEM_1)
                add_scancode(47, 0x27, {";", "semicolon", "colon"});
                // Home row second right of 'L' (' / İ / OEM_7)
                add_scancode(48, 0x28, {"'", "quote", "apostrophe", "singlequote"});
                // ISO key next to Enter or ANSI backslash above Enter (# / , / \ / OEM_5 / OEM_2)
                add_scancode(51, 0x2B, {"#", "pound", "hash", "numbersign", "\\", "backslash", "ansibackslash", "\\(ansi)"});
                // ISO key between Left Shift and Z (\ (ISO) / < > | / OEM_102)
                add_scancode(94, 0x56, {"\\(iso)", "isobackslash", "<", "less", "nonusbackslash"});
                // Bottom row right of 'M' (, / Ö / OEM_COMMA)
                add_scancode(59, 0x33, {",", "comma"});
                // Bottom row second right of 'M' (. / Ç / OEM_PERIOD)
                add_scancode(60, 0x34, {".", "period", "dot"});
                // Bottom row left of Right Shift (/ / . / OEM_2)
                add_scancode(61, 0x35, {"/", "forwardslash", "slash", "?"});

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

        std::vector<std::shared_ptr<DImpl>> deviceList() {
            std::vector<std::shared_ptr<DImpl>> list;
            for(auto &d : S().devices)
                list.push_back(Access::impl(*d));

            return list;
        }

        bool isDirectMode(const LL::Mode &m) {
            return String::ToLower(m.name) == "direct";
        }

        // ---------------------------------------------------------
        // Layer management helpers (state_mtx must be held)
        // ---------------------------------------------------------

        DeviceLayer makeDefaultLayer(const DImpl &d) {
            DeviceLayer l;
            l.touched     = false;
            l.mode_dirty  = false;
            l.active_mode = d.orig_mode;
            l.modes       = d.default_modes;
            l.base_leds   = d.initial_base_leds;
            return l;
        }

        /// Returns a mutable DeviceLayer for `ctx`, lazily creating it if it doesn't exist yet.
        DeviceLayer &getOrCreateLayer(DImpl &d, const Gorgon::Animation::Governor *ctx) {
            if(!ctx)
                return d.global_layer;

            auto it = d.gov_layers.find(ctx);
            if(it == d.gov_layers.end())
                it = d.gov_layers.emplace(ctx, makeDefaultLayer(d)).first;

            return it->second;
        }

        /// Returns the const DeviceLayer for a specific handle context (falls back to global_layer if not created yet)
        const DeviceLayer &getHandleLayer(const DImpl &d, const Gorgon::Animation::Governor *ctx) {
            if(!ctx)
                return d.global_layer;

            auto it = d.gov_layers.find(ctx);
            if(it != d.gov_layers.end())
                return it->second;

            return d.global_layer;
        }

        /// Resolves which DeviceLayer currently drives physical hardware for device `d`,
        /// or returns nullptr if neither the active governor nor the global layer has touched `d`.
        DeviceLayer *resolveActiveLayer(DImpl &d, const Gorgon::Animation::Governor **out_ctx = nullptr) {
            const Gorgon::Animation::Governor *active_gov = &Gorgon::Animation::Governor::Active();

            auto it = d.gov_layers.find(active_gov);
            if(it != d.gov_layers.end() && it->second.touched) {
                if(out_ctx) *out_ctx = active_gov;
                return &it->second;
            }

            if(d.global_layer.touched) {
                if(out_ctx) *out_ctx = nullptr;
                return &d.global_layer;
            }

            if(out_ctx) *out_ctx = nullptr;
            return nullptr;
        }

        void touchLayer(DImpl &d, DeviceLayer &layer) {
            if(!layer.touched && d.direct_mode >= 0) {
                layer.active_mode = d.direct_mode;
            }
            layer.touched = true;
            d.dirty       = true;
            d.revision++;
        }

        void markAllDevicesDirtyLocked() {
            for(auto &d : deviceList()) {
                d->dirty = true;
                d->revision++;
            }
            if(!S().suspended.load())
                S().wake.notify_all();
        }

        void setLED(DImpl &d, const Gorgon::Animation::Governor *ctx, int idx, const Graphics::RGBA &c) {
            if(idx < 0 || idx >= (int)d.leds.size())
                return;

            auto &layer = getOrCreateLayer(d, ctx);
            if(idx >= (int)layer.base_leds.size())
                layer.base_leds.resize(d.leds.size(), Graphics::RGBA(Byte(0), Byte(0), Byte(0), Byte(255)));

            auto col = opaque(c);
            bool direct = (layer.active_mode == d.direct_mode);
            if(layer.touched && direct && layer.base_leds[idx] == col)
                return;

            layer.base_leds[idx] = col;
            if(d.direct_mode >= 0)
                layer.active_mode = d.direct_mode;

            if(!ctx)
                d.leds[idx].color = col;

            touchLayer(d, layer);
        }

        Graphics::RGBA getLED(const DImpl &d, const Gorgon::Animation::Governor *ctx, int idx) {
            if(idx < 0 || idx >= (int)d.leds.size())
                return transparent;

            const auto &layer = getHandleLayer(d, ctx);
            if(idx < (int)layer.base_leds.size())
                return layer.base_leds[idx];

            return d.leds[idx].color;
        }

        // ---------------------------------------------------------
        // Progressbar & Animation Compositing Helpers
        // ---------------------------------------------------------

        void appendZoneMatrixColumns(const ZImpl &zi, std::vector<std::vector<int>> &out_positions) {
            if(zi.width > 0 && zi.height > 0) {
                for(int x = 0; x < zi.width; x++) {
                    std::vector<int> col_leds;
                    for(int y = 0; y < zi.height; y++) {
                        int local = zi.matrix[(size_t)y * zi.width + x];
                        if(local >= 0 && local < zi.led_count)
                            col_leds.push_back(zi.led_offset + local);
                    }
                    if(!col_leds.empty())
                        out_positions.push_back(std::move(col_leds));
                }
            }
            else {
                for(int i = 0; i < zi.led_count; i++)
                    out_positions.push_back({zi.led_offset + i});
            }
        }

        void appendDeviceProgressPositions(const DImpl &d, std::vector<std::vector<int>> &out_positions) {
            if(d.matrix_zone >= 0 && d.matrix_zone < (int)d.zones.size()) {
                if(auto zi = Access::impl(d.zones[d.matrix_zone])) {
                    appendZoneMatrixColumns(*zi, out_positions);
                    return;
                }
            }
            for(int i = 0; i < (int)d.leds.size(); i++)
                out_positions.push_back({i});
        }

        std::vector<int> resolveProgressPositionsForDevice(const PImpl &p, const DImpl &d, std::vector<std::vector<int>> &out_positions) {
            out_positions.clear();
            switch(p.target) {
            case PImpl::Target::AllKeyboards:
                if(d.is_keyboard && d.connected) {
                    for(auto key : p.keys.keys) {
                        auto it = d.keymap.find(key);
                        if(it != d.keymap.end() && !it->second.empty())
                            out_positions.push_back(it->second);
                    }
                }
                break;

            case PImpl::Target::AllStrips:
                if(d.connected) {
                    if(d.type == DeviceType::LedStrip) {
                        appendDeviceProgressPositions(d, out_positions);
                    }
                    else {
                        for(const auto &z : d.zones) {
                            if(z.GetType() == ZoneType::Linear) {
                                if(auto zi = Access::impl(z)) {
                                    appendZoneMatrixColumns(*zi, out_positions);
                                }
                            }
                        }
                    }
                }
                break;

            case PImpl::Target::Keyboard:
                if(auto td = p.device.lock()) {
                    if(td.get() == &d && d.connected) {
                        for(auto key : p.keys.keys) {
                            auto it = d.keymap.find(key);
                            if(it != d.keymap.end() && !it->second.empty())
                                out_positions.push_back(it->second);
                        }
                    }
                }
                break;

            case PImpl::Target::Device:
                if(auto td = p.device.lock()) {
                    if(td.get() == &d && d.connected) {
                        if(p.use_keys && d.is_keyboard) {
                            for(auto key : p.keys.keys) {
                                auto it = d.keymap.find(key);
                                if(it != d.keymap.end() && !it->second.empty())
                                    out_positions.push_back(it->second);
                            }
                        }
                        else {
                            appendDeviceProgressPositions(d, out_positions);
                        }
                    }
                }
                break;

            case PImpl::Target::Zone:
                if(p.zone) {
                    if(auto td = p.zone->dev.lock()) {
                        if(td.get() == &d && d.connected) {
                            int begin = p.zone->led_offset, end = begin + p.zone->led_count;
                            if(p.use_keys) {
                                for(auto key : p.keys.keys) {
                                    auto it = d.keymap.find(key);
                                    if(it == d.keymap.end())
                                        continue;
                                    std::vector<int> leds;
                                    for(int l : it->second) {
                                        if(l >= begin && l < end)
                                            leds.push_back(l);
                                    }
                                    if(!leds.empty())
                                        out_positions.push_back(std::move(leds));
                                }
                            }
                            else {
                                appendZoneMatrixColumns(*p.zone, out_positions);
                            }
                        }
                    }
                }
                break;

            default:
                break;
            }
            return {};
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

        void compositeProgressbar(const PImpl &p, const DImpl &d, std::vector<Graphics::RGBA> &colors) {
            if(!p.active)
                return;

            std::vector<std::vector<int>> positions;
            resolveProgressPositionsForDevice(p, d, positions);
            if(positions.empty())
                return;

            auto fg = levelColor(p);
            int n = (int)positions.size();

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

                for(int led : positions[i]) {
                    if(led < 0 || led >= (int)colors.size())
                        continue;

                    Graphics::RGBA base_c = colors[led];
                    if(p.bg.A != 0)
                        base_c = lerp(base_c, p.bg, p.bg.A / 255.f);

                    colors[led] = lerp(base_c, fg, t);
                }
            }
        }

        std::optional<Geometry::Point> findKeyMatrixCoord(const DImpl &d, Input::Key key) {
            if(d.matrix_zone < 0 || d.matrix_zone >= (int)d.zones.size())
                return std::nullopt;

            auto it = d.keymap.find(key);
            if(it == d.keymap.end() || it->second.empty())
                return std::nullopt;

            auto zi = Access::impl(d.zones[d.matrix_zone]);
            if(!zi || zi->width <= 0 || zi->height <= 0)
                return std::nullopt;

            int target_local = it->second[0] - zi->led_offset;
            if(target_local < 0 || target_local >= zi->led_count)
                return std::nullopt;

            int sum_x = 0, sum_y = 0, count = 0;
            for(int y = 0; y < zi->height; y++) {
                for(int x = 0; x < zi->width; x++) {
                    if(zi->matrix[(size_t)y * zi->width + x] == target_local) {
                        sum_x += x;
                        sum_y += y;
                        count++;
                    }
                }
            }

            if(count == 0)
                return std::nullopt;

            return Geometry::Point(sum_x / count, sum_y / count);
        }

        /// Finds the normalized horizontal position [0..1] of `key` on any connected keyboard matrix,
        /// allowing key-originated effects (ExplosionEffect::SetOriginKey, ReactiveEffect) to map onto width x 1 strips.
        std::optional<float> findKeyNormalizedX(Input::Key key) {
            for(auto &dev_ptr : S().devices) {
                auto kd = Access::impl(*dev_ptr);
                if(kd && kd->is_keyboard && kd->connected && kd->matrix_zone >= 0 && kd->matrix_zone < (int)kd->zones.size()) {
                    if(auto pt = findKeyMatrixCoord(*kd, key)) {
                        auto zi = Access::impl(kd->zones[kd->matrix_zone]);
                        if(zi && zi->width > 1)
                            return Clamp((float)pt->X / (float)(zi->width - 1), 0.f, 1.f);
                        return 0.5f;
                    }
                }
            }
            return std::nullopt;
        }

        Graphics::RGBA evalStops(const std::vector<Graphics::RGBA> &stops, float t) {
            if(stops.empty())
                return transparent;
            if(stops.size() == 1)
                return stops[0];

            t = Clamp(t, 0.f, 1.f);
            float scaled = t * (stops.size() - 1);
            size_t idx = std::min((size_t)std::floor(scaled), stops.size() - 2);
            float frac = scaled - (float)idx;
            return lerpRGBA(stops[idx], stops[idx + 1], frac);
        }

        bool animationTargetsDevice(const AImpl &a, const DImpl &d) {
            switch(a.target) {
            case AImpl::Target::AllDevices:
                return d.connected;
            case AImpl::Target::AllKeyboards:
                return d.is_keyboard && d.connected;
            case AImpl::Target::AllStrips:
                if(!d.connected) return false;
                if(d.type == DeviceType::LedStrip) return true;
                for(const auto &z : d.zones) {
                    if(z.GetType() == ZoneType::Linear) return true;
                }
                return false;
            case AImpl::Target::Device:
            case AImpl::Target::Keyboard:
                if(auto td = a.device.lock())
                    return td.get() == &d && d.connected;
                return false;
            case AImpl::Target::Zone:
                if(a.zone) {
                    if(auto td = a.zone->dev.lock())
                        return td.get() == &d && d.connected;
                }
                return false;
            }
            return false;
        }

        std::vector<int> resolveAnimationTargetLEDs(const AImpl &a, const DImpl &d) {
            std::vector<int> leds;
            if(a.target == AImpl::Target::AllStrips && d.type != DeviceType::LedStrip) {
                for(const auto &z : d.zones) {
                    if(z.GetType() == ZoneType::Linear) {
                        for(int i = 0; i < z.GetLEDCount(); i++)
                            leds.push_back(z.GetLEDOffset() + i);
                    }
                }
                return leds;
            }

            int begin = 0, end = (int)d.leds.size();
            if(a.target == AImpl::Target::Zone && a.zone) {
                begin = a.zone->led_offset;
                end   = begin + a.zone->led_count;
            }

            if(a.use_keys && d.is_keyboard) {
                for(auto key : a.keys.keys) {
                    auto it = d.keymap.find(key);
                    if(it == d.keymap.end())
                        continue;
                    for(int l : it->second) {
                        if(l >= begin && l < end)
                            leds.push_back(l);
                    }
                }
            }
            else {
                leds.reserve(std::max(0, end - begin));
                for(int i = begin; i < end; i++)
                    leds.push_back(i);
            }
            return leds;
        }

        /// Returns the matrix/strip zones on `d` that spatial effects (Explosion, Swipe, Reactive) should evaluate on.
        /// Both 2D keyboard matrices (width x height) and 1D linear strips (width x 1) are returned as ZImpl matrices.
        std::vector<std::shared_ptr<ZImpl>> resolveTargetMatrixZones(const AImpl &a, const DImpl &d) {
            std::vector<std::shared_ptr<ZImpl>> out;
            if(a.target == AImpl::Target::Zone) {
                if(a.zone && a.zone->width > 0 && a.zone->height > 0)
                    out.push_back(a.zone);
                return out;
            }

            if(a.target == AImpl::Target::AllStrips && d.type != DeviceType::LedStrip) {
                for(const auto &z : d.zones) {
                    if(z.GetType() == ZoneType::Linear) {
                        auto zi = Access::impl(z);
                        if(zi && zi->width > 0 && zi->height > 0)
                            out.push_back(std::move(zi));
                    }
                }
                return out;
            }

            if(d.is_keyboard && d.matrix_zone >= 0 && d.matrix_zone < (int)d.zones.size()) {
                auto zi = Access::impl(d.zones[d.matrix_zone]);
                if(zi && zi->width > 0 && zi->height > 0)
                    out.push_back(std::move(zi));
                return out;
            }

            for(const auto &z : d.zones) {
                auto zi = Access::impl(z);
                if(zi && zi->width > 0 && zi->height > 0)
                    out.push_back(std::move(zi));
            }
            return out;
        }

        void compositeAnimation(const AImpl &a, const DImpl &d, std::vector<Graphics::RGBA> &colors) {
            if(!a.visible || !animationTargetsDevice(a, d))
                return;

            float dur = (float)std::max(1u, a.duration_ms);
            float t   = Clamp(a.current_progress_ms / dur, 0.f, 1.f);

            // Uniform per-target-LED effects: Fade, Pulse, Blink
            if(a.type == AImpl::Type::Fade || a.type == AImpl::Type::Pulse || a.type == AImpl::Type::Blink) {
                Graphics::RGBA overlay = transparent;
                if(a.type == AImpl::Type::Fade) {
                    overlay = evalStops(a.stops, t);
                }
                else if(a.type == AImpl::Type::Pulse) {
                    Graphics::RGBA peak   = a.stops.size() > 0 ? a.stops[0] : Graphics::RGBA(0xffffffffu);
                    Graphics::RGBA trough = a.stops.size() > 1 ? a.stops[1] : transparent;
                    // Smooth sine wave: 0 -> 1 -> 0 over period
                    float wave = 0.5f * (1.f - std::cos(t * 2.f * 3.14159265f));
                    overlay = lerpRGBA(trough, peak, wave);
                }
                else if(a.type == AImpl::Type::Blink) {
                    Graphics::RGBA on_col  = a.stops.size() > 0 ? a.stops[0] : Graphics::RGBA(0xffffffffu);
                    Graphics::RGBA off_col = a.stops.size() > 1 ? a.stops[1] : transparent;
                    overlay = (t < a.duty_cycle) ? on_col : off_col;
                }

                for(int led : resolveAnimationTargetLEDs(a, d)) {
                    if(led >= 0 && led < (int)colors.size())
                        colors[led] = blendColor(colors[led], overlay, a.blend);
                }
                return;
            }

            // Spatial ExplosionEffect (evaluates over 2D matrices and width x 1 strips)
            if(a.type == AImpl::Type::Explosion) {
                auto mzones = resolveTargetMatrixZones(a, d);
                Graphics::RGBA base_col = a.stops.empty() ? Graphics::RGBA(0xffff4000u) : a.stops[0];
                float fade_out = 1.f - t * t; // Smooth intensity falloff as explosion expands

                if(!mzones.empty()) {
                    for(const auto &mz : mzones) {
                        float cx = (mz->width > 1) ? a.origin_norm.X * (mz->width - 1) : 0.f;
                        float cy = (mz->height > 1) ? a.origin_norm.Y * (mz->height - 1) : 0.f;

                        if(a.origin_mode == AImpl::OriginMode::Point) {
                            cx = (float)a.origin_pt.X;
                            cy = (mz->height > 1) ? (float)a.origin_pt.Y : 0.f;
                        }
                        else if(a.origin_mode == AImpl::OriginMode::Key) {
                            if(auto kp = findKeyMatrixCoord(d, a.origin_key)) {
                                cx = (float)kp->X;
                                cy = (float)kp->Y;
                            }
                            else if(auto nx = findKeyNormalizedX(a.origin_key)) {
                                cx = *nx * std::max(0, mz->width - 1);
                                cy = 0.f;
                            }
                        }

                        float max_r = a.max_radius;
                        if(max_r <= 0.f) {
                            float dx = std::max(cx, (float)(mz->width - 1) - cx);
                            float dy = std::max(cy, (float)(mz->height - 1) - cy);
                            max_r = std::sqrt(dx * dx + dy * dy) + std::max(1.f, a.ring_thickness);
                        }

                        float cur_r = t * max_r;
                        float half_w = std::max(0.5f, a.ring_thickness * 0.5f);

                        for(int y = 0; y < mz->height; y++) {
                            for(int x = 0; x < mz->width; x++) {
                                int local = mz->matrix[(size_t)y * mz->width + x];
                                if(local < 0)
                                    continue;

                                int gi = mz->led_offset + local;
                                if(gi < 0 || gi >= (int)colors.size())
                                    continue;

                                float dist = std::hypot((float)x - cx, (float)y - cy);
                                float intensity = 0.f;
                                if(a.ring_thickness <= 0.f) {
                                    // Filled expanding disc
                                    if(dist <= cur_r)
                                        intensity = 1.f - 0.5f * (dist / std::max(1.f, cur_r));
                                }
                                else {
                                    float d_ring = std::abs(dist - cur_r);
                                    if(d_ring < half_w)
                                        intensity = 1.f - (d_ring / half_w);
                                }

                                intensity *= fade_out;
                                if(intensity > 0.001f) {
                                    Graphics::RGBA px = base_col;
                                    px.A = Byte(Clamp(base_col.A * intensity, 0.f, 255.f));
                                    colors[gi] = blendColor(colors[gi], px, a.blend);
                                }
                            }
                        }
                    }
                }
                else {
                    // 1D fallback for zoneless devices
                    auto leds = resolveAnimationTargetLEDs(a, d);
                    int n = (int)leds.size();
                    if(n > 0) {
                        float center = a.origin_norm.X * std::max(0, n - 1);
                        float max_r  = std::max(center, (float)(n - 1) - center) + std::max(1.f, a.ring_thickness);
                        float cur_r  = t * max_r;
                        float half_w = std::max(0.5f, a.ring_thickness * 0.5f);

                        for(int i = 0; i < n; i++) {
                            float dist = std::abs((float)i - center);
                            float d_ring = std::abs(dist - cur_r);
                            if(d_ring < half_w) {
                                float intensity = (1.f - d_ring / half_w) * fade_out;
                                Graphics::RGBA px = base_col;
                                px.A = Byte(Clamp(base_col.A * intensity, 0.f, 255.f));
                                colors[leds[i]] = blendColor(colors[leds[i]], px, a.blend);
                            }
                        }
                    }
                }
                return;
            }

            // Spatial SwipeEffect (evaluates over 2D matrices and width x 1 strips)
            if(a.type == AImpl::Type::Swipe) {
                auto mzones = resolveTargetMatrixZones(a, d);
                Graphics::RGBA base_col = a.stops.empty() ? Graphics::RGBA(0xff00ffffu) : a.stops[0];
                float bw = std::max(0.05f, a.band_width);
                float pos = -bw + t * (1.f + 2.f * bw);

                if(!mzones.empty()) {
                    for(const auto &mz : mzones) {
                        for(int y = 0; y < mz->height; y++) {
                            for(int x = 0; x < mz->width; x++) {
                                int local = mz->matrix[(size_t)y * mz->width + x];
                                if(local < 0)
                                    continue;

                                int gi = mz->led_offset + local;
                                if(gi < 0 || gi >= (int)colors.size())
                                    continue;

                                float coord = 0.f;
                                switch(a.swipe_dir) {
                                case SwipeDirection::LeftToRight:
                                    coord = mz->width > 1 ? (float)x / (mz->width - 1) : 0.5f;
                                    break;
                                case SwipeDirection::RightToLeft:
                                    coord = mz->width > 1 ? 1.f - (float)x / (mz->width - 1) : 0.5f;
                                    break;
                                case SwipeDirection::TopToBottom:
                                    coord = mz->height > 1 ? (float)y / (mz->height - 1) : 0.5f;
                                    break;
                                case SwipeDirection::BottomToTop:
                                    coord = mz->height > 1 ? 1.f - (float)y / (mz->height - 1) : 0.5f;
                                    break;
                                }

                                float dist = std::abs(coord - pos);
                                if(dist < bw) {
                                    float intensity = 0.5f * (1.f + std::cos((dist / bw) * 3.14159265f));
                                    Graphics::RGBA px = base_col;
                                    px.A = Byte(Clamp(base_col.A * intensity, 0.f, 255.f));
                                    colors[gi] = blendColor(colors[gi], px, a.blend);
                                }
                            }
                        }
                    }
                }
                else {
                    auto leds = resolveAnimationTargetLEDs(a, d);
                    int n = (int)leds.size();
                    for(int i = 0; i < n; i++) {
                        float u = n > 1 ? (float)i / (n - 1) : 0.5f;
                        if(a.swipe_dir == SwipeDirection::RightToLeft || a.swipe_dir == SwipeDirection::BottomToTop)
                            u = 1.f - u;

                        float dist = std::abs(u - pos);
                        if(dist < bw) {
                            float intensity = 0.5f * (1.f + std::cos((dist / bw) * 3.14159265f));
                            Graphics::RGBA px = base_col;
                            px.A = Byte(Clamp(base_col.A * intensity, 0.f, 255.f));
                            colors[leds[i]] = blendColor(colors[leds[i]], px, a.blend);
                        }
                    }
                }
                return;
            }

            // ReactiveEffect (per-key press fade & matrix/strip ripple)
            if(a.type == AImpl::Type::Reactive) {
                auto mzones = resolveTargetMatrixZones(a, d);

                for(const auto &kp : a.active_presses) {
                    unsigned age = (a.reactive_clock_ms >= kp.start_ms) ? (a.reactive_clock_ms - kp.start_ms) : 0;
                    if(age >= a.duration_ms)
                        continue;

                    float ratio = 1.f - Clamp((float)age / dur, 0.f, 1.f);
                    Graphics::RGBA px = kp.color;
                    px.A = Byte(Clamp(kp.color.A * ratio, 0.f, 255.f));

                    if(d.is_keyboard) {
                        auto it = d.keymap.find(kp.key);
                        if(it != d.keymap.end()) {
                            for(int led : it->second) {
                                if(led >= 0 && led < (int)colors.size())
                                    colors[led] = blendColor(colors[led], px, a.blend);
                            }
                        }

                        if(a.ripple_radius > 0.f) {
                            if(auto pt = findKeyMatrixCoord(d, kp.key)) {
                                float cur_r = (1.f - ratio) * a.ripple_radius;
                                for(const auto &mz : mzones) {
                                    for(int y = 0; y < mz->height; y++) {
                                        for(int x = 0; x < mz->width; x++) {
                                            int local = mz->matrix[(size_t)y * mz->width + x];
                                            if(local < 0) continue;
                                            int gi = mz->led_offset + local;
                                            if(gi < 0 || gi >= (int)colors.size()) continue;

                                            float dist = std::hypot((float)x - pt->X, (float)y - pt->Y);
                                            if(dist > 0.5f && std::abs(dist - cur_r) < 1.25f) {
                                                float r_int = (1.f - std::abs(dist - cur_r) / 1.25f) * ratio;
                                                Graphics::RGBA rpx = kp.color;
                                                rpx.A = Byte(Clamp(kp.color.A * r_int, 0.f, 255.f));
                                                colors[gi] = blendColor(colors[gi], rpx, a.blend);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if(!mzones.empty()) {
                        // Non-keyboard matrix / width x 1 strip: project key onto matrix X coordinate
                        auto norm_x = findKeyNormalizedX(kp.key);
                        for(const auto &mz : mzones) {
                            int sx = 0;
                            if(mz->width > 1) {
                                if(norm_x)
                                    sx = std::clamp((int)std::round(*norm_x * (mz->width - 1)), 0, mz->width - 1);
                                else
                                    sx = std::abs((int)kp.key) % mz->width;
                            }
                            int sy = (mz->height > 1) ? (mz->height / 2) : 0;

                            int center_local = mz->matrix[(size_t)sy * mz->width + sx];
                            if(center_local >= 0) {
                                int gi = mz->led_offset + center_local;
                                if(gi >= 0 && gi < (int)colors.size())
                                    colors[gi] = blendColor(colors[gi], px, a.blend);
                            }

                            if(a.ripple_radius > 0.f) {
                                float cur_r = (1.f - ratio) * a.ripple_radius;
                                for(int y = 0; y < mz->height; y++) {
                                    for(int x = 0; x < mz->width; x++) {
                                        int local = mz->matrix[(size_t)y * mz->width + x];
                                        if(local < 0) continue;
                                        int gi = mz->led_offset + local;
                                        if(gi < 0 || gi >= (int)colors.size()) continue;

                                        float dist = std::hypot((float)x - sx, (float)y - sy);
                                        if(dist > 0.5f && std::abs(dist - cur_r) < 1.25f) {
                                            float r_int = (1.f - std::abs(dist - cur_r) / 1.25f) * ratio;
                                            Graphics::RGBA rpx = kp.color;
                                            rpx.A = Byte(Clamp(kp.color.A * r_int, 0.f, 255.f));
                                            colors[gi] = blendColor(colors[gi], rpx, a.blend);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        void compositeShortcutOverlay(const internal::ShortcutOverlay &o, const DImpl &d, std::vector<Graphics::RGBA> &colors) {
            if(o.mode == ModifierHighlightMode::Disabled || o.bindings.empty() || !d.is_keyboard)
                return;

            auto current_mod = Input::Keyboard::CurrentModifier;

            auto applyKey = [&](Input::Key k, const Graphics::RGBA &col) {
                if(col.A == 0) return;
                auto it = d.keymap.find(k);
                if(it != d.keymap.end()) {
                    for(int led_idx : it->second) {
                        if(led_idx >= 0 && led_idx < (int)colors.size()) {
                            colors[led_idx] = blendColor(colors[led_idx], col, BlendMode::AlphaBlend);
                        }
                    }
                }
            };

            if(o.mode == ModifierHighlightMode::ActiveModifiers) {
                if(current_mod != Input::Keyboard::Modifier::None) {
                    // 1. When a modifier combination is held, illuminate its matching action keys and 
                    // further highlight the modifier keys that can be combined with the current modifier.
                    for(const auto &b : o.bindings) {
                        if(b.modifier == current_mod && b.color.A > 0) {
                            applyKey(b.key, b.color);
                        }
                    }

                    // Split and highlight the modifier keys that are part of the current modifier combination.
                    if(current_mod & Input::Keyboard::Modifier::Ctrl) {
                        applyKey(Input::Keyboard::Keycodes::Control, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RControl, o.modifier_key_color);
                    }
                    if(current_mod & Input::Keyboard::Modifier::Shift) {
                        applyKey(Input::Keyboard::Keycodes::Shift, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RShift, o.modifier_key_color);
                    }
                    if(current_mod & Input::Keyboard::Modifier::Alt) {
                        applyKey(Input::Keyboard::Keycodes::Alt, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RAlt, o.modifier_key_color);
                    }
                    if(current_mod & Input::Keyboard::Modifier::Meta) {
                        applyKey(Input::Keyboard::Keycodes::Meta, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RMeta, o.modifier_key_color);
                    }

                    bool has_ctrl = false, has_shift = false, has_alt = false, has_meta = false;
                    for(const auto &b : o.bindings) {
                        if(b.modifier == current_mod) continue;

                        if((b.modifier & current_mod) && (b.modifier & Input::Keyboard::Modifier::Ctrl))  has_ctrl = true;
                        if((b.modifier & current_mod) && (b.modifier & Input::Keyboard::Modifier::Shift)) has_shift = true;
                        if((b.modifier & current_mod) && (b.modifier & Input::Keyboard::Modifier::Alt))   has_alt = true;
                        if((b.modifier & current_mod) && (b.modifier & Input::Keyboard::Modifier::Meta))  has_meta = true;
                    }

                    if(has_ctrl) {
                        applyKey(Input::Keyboard::Keycodes::Control, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RControl, o.modifier_key_color);
                    }
                    if(has_shift) {
                        applyKey(Input::Keyboard::Keycodes::Shift, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RShift, o.modifier_key_color);
                    }
                    if(has_alt) {
                        applyKey(Input::Keyboard::Keycodes::Alt, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RAlt, o.modifier_key_color);
                    }
                    if(has_meta) {
                        applyKey(Input::Keyboard::Keycodes::Meta, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RMeta, o.modifier_key_color);
                    }
                }
                else {

                    // 2. Highlight only the modifier keys that have shortcuts registered.
                    // For instance if there are no shortcuts with Meta key, it will not be highlighted.
                    // Highlight the ones that have shortcuts registered.
                    bool has_ctrl = false, has_shift = false, has_alt = false, has_meta = false;
                    for(const auto &b : o.bindings) {
                        if(b.modifier & Input::Keyboard::Modifier::Ctrl)  has_ctrl = true;
                        if(b.modifier & Input::Keyboard::Modifier::Shift && !(
                        b.modifier & Input::Keyboard::Modifier::Ctrl)) has_shift = true;
                        if(b.modifier & Input::Keyboard::Modifier::Alt)   has_alt = true;
                        if(b.modifier & Input::Keyboard::Modifier::Meta)  has_meta = true;
                    }

                    if(has_ctrl) {
                        applyKey(Input::Keyboard::Keycodes::Control, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RControl, o.modifier_key_color);
                    }
                    if(has_shift) {
                        applyKey(Input::Keyboard::Keycodes::Shift, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RShift, o.modifier_key_color);
                    }
                    if(has_alt) {
                        applyKey(Input::Keyboard::Keycodes::Alt, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RAlt, o.modifier_key_color);
                    }
                    if(has_meta) {
                        applyKey(Input::Keyboard::Keycodes::Meta, o.modifier_key_color);
                        applyKey(Input::Keyboard::Keycodes::RMeta, o.modifier_key_color);
                    }

                    // 3. Highlight the action keys that do not have any modifiers
                    for(const auto &b : o.bindings) {
                        if(b.modifier == Input::Keyboard::Modifier::None && b.color.A > 0) {
                            applyKey(b.key, b.color);
                        }
                    }
                }
            }
            else if(o.mode == ModifierHighlightMode::RegularKeys) {
                bool has_ctrl = false, has_shift = false, has_alt = false, has_meta = false;
                for(const auto &b : o.bindings) {
                    if(b.color.A > 0) {
                        applyKey(b.key, b.color);
                        if(b.modifier & Input::Keyboard::Modifier::Ctrl)  has_ctrl = true;
                        if(b.modifier & Input::Keyboard::Modifier::Shift) has_shift = true;
                        if(b.modifier & Input::Keyboard::Modifier::Alt)   has_alt = true;
                        if(b.modifier & Input::Keyboard::Modifier::Meta)  has_meta = true;
                    }
                }
                if(has_ctrl) {
                    applyKey(Input::Keyboard::Keycodes::Control, o.modifier_key_color);
                    applyKey(Input::Keyboard::Keycodes::RControl, o.modifier_key_color);
                }
                if(has_shift) {
                    applyKey(Input::Keyboard::Keycodes::Shift, o.modifier_key_color);
                    applyKey(Input::Keyboard::Keycodes::RShift, o.modifier_key_color);
                }
                if(has_alt) {
                    applyKey(Input::Keyboard::Keycodes::Alt, o.modifier_key_color);
                    applyKey(Input::Keyboard::Keycodes::RAlt, o.modifier_key_color);
                }
                if(has_meta) {
                    applyKey(Input::Keyboard::Keycodes::Meta, o.modifier_key_color);
                    applyKey(Input::Keyboard::Keycodes::RMeta, o.modifier_key_color);
                }
            }
        }

        /// Composes the final RGBA colors for a device in a given layer context (`state_mtx` must be held)
        std::vector<Graphics::RGBA> composeRGBA(const DImpl &d, const DeviceLayer &layer, const Gorgon::Animation::Governor *layer_ctx) {
            std::vector<Graphics::RGBA> colors = layer.base_leds;
            if(colors.size() < d.leds.size())
                colors.resize(d.leds.size(), Graphics::RGBA(Byte(0), Byte(0), Byte(0), Byte(255)));

            auto &s = S();
            const Gorgon::Animation::Governor *active_gov = &Gorgon::Animation::Governor::Active();

            // 1. Composite Progressbars belonging to this layer (or global progressbars if global layer is showing)
            for(auto &wp : s.progressbars) {
                if(auto p = wp.lock()) {
                    if(p->context == layer_ctx || (layer_ctx == nullptr && p->context == active_gov))
                        compositeProgressbar(*p, d, colors);
                }
            }

            // 2. Composite Animations belonging to this layer (or active governor animations over global fallback base)
            for(auto &wa : s.animations) {
                if(auto a = wa.lock()) {
                    if(a->context == layer_ctx || (layer_ctx == nullptr && a->context == active_gov))
                        compositeAnimation(*a, d, colors);
                }
            }

            // 3. Composite Shortcut Overlays belonging to this layer (or active governor shortcuts over global fallback base)
            for(auto &wo : s.shortcut_overlays) {
                if(auto o = wo.lock()) {
                    if(o->active && (o->context == active_gov || o->context == nullptr || o->context == layer_ctx))
                        compositeShortcutOverlay(*o, d, colors);
                }
            }

            return colors;
        }

        std::vector<LL::Color> toWireColors(const std::vector<Graphics::RGBA> &rgba) {
            std::vector<LL::Color> out;
            out.reserve(rgba.size());
            for(const auto &c : rgba)
                out.push_back(wire(c));
            return out;
        }

        void seedDefaultModeColors(std::vector<LL::Mode> &modes, const LL::Mode &orig_mode_data, const std::vector<LL::Color> &orig_colors = {}) {
            static const LL::Color palette[] = {
                LL::Color::FromRGB(255, 0, 0),
                LL::Color::FromRGB(255, 128, 0),
                LL::Color::FromRGB(255, 230, 0),
                LL::Color::FromRGB(0, 230, 60),
                LL::Color::FromRGB(0, 210, 255),
                LL::Color::FromRGB(40, 80, 255),
                LL::Color::FromRGB(180, 0, 255),
                LL::Color::FromRGB(255, 0, 140)
            };
            constexpr size_t palette_sz = sizeof(palette) / sizeof(palette[0]);

            LL::Color primary_fallback = LL::Color::FromRGB(0, 200, 255);
            bool found_primary = false;
            for(const auto &c : orig_mode_data.colors) {
                if(c.ToWireRGB() != 0) {
                    primary_fallback = c;
                    found_primary = true;
                    break;
                }
            }
            if(!found_primary) {
                for(const auto &c : orig_colors) {
                    if(c.ToWireRGB() != 0) {
                        primary_fallback = c;
                        found_primary = true;
                        break;
                    }
                }
            }

            for(auto &m : modes) {
                if((m.flags & LL::ModeFlags::HasModeSpecificColor) == 0) {
                    if((m.flags & LL::ModeFlags::HasPerLedColor) != 0 && !isDirectMode(m)) {
                        if((m.flags & LL::ModeFlags::HasRandomColor) == 0 || m.color_mode != static_cast<uint32_t>(LL::ModeColorMode::Random)) {
                            m.color_mode = static_cast<uint32_t>(LL::ModeColorMode::PerLed);
                        }
                    }
                    continue;
                }

                if(m.colors.empty() && m.colors_min > 0)
                    m.colors.resize(m.colors_min, LL::Color(0, 0, 0, 0));

                if(m.colors.empty() && m.colors_max > 0)
                    m.colors.resize(1, LL::Color(0, 0, 0, 0));

                bool any_non_black = false;
                for(const auto &c : m.colors) {
                    if(c.ToWireRGB() != 0) {
                        any_non_black = true;
                        break;
                    }
                }

                if(!any_non_black && !m.colors.empty()) {
                    if(m.colors.size() == 1) {
                        m.colors[0] = primary_fallback;
                    }
                    else {
                        for(size_t i = 0; i < m.colors.size(); i++)
                            m.colors[i] = palette[i % palette_sz];
                    }
                }

                if((m.flags & LL::ModeFlags::HasRandomColor) == 0 || m.color_mode != static_cast<uint32_t>(LL::ModeColorMode::Random)) {
                    m.color_mode = static_cast<uint32_t>(LL::ModeColorMode::ModeSpecific);
                }
            }
        }

        // ---------------------------------------------------------
        // Governor lifecycle hooks
        // ---------------------------------------------------------

        void onGovernorActivated(const Gorgon::Animation::Governor *) {
            auto &s = S();
            if(!s.initialized)
                return;

            std::lock_guard<std::mutex> sg(s.state_mtx);
            markAllDevicesDirtyLocked();
        }

        void onGovernorDestroyed(const Gorgon::Animation::Governor *gov) {
            auto &s = S();
            if(!s.initialized || !gov)
                return;

            std::lock_guard<std::mutex> sg(s.state_mtx);
            for(auto &d : deviceList()) {
                if(d->gov_layers.erase(gov) > 0) {
                    d->dirty = true;
                    d->revision++;
                }
            }
            for(auto &wp : s.progressbars) {
                if(auto p = wp.lock()) {
                    if(p->context == gov)
                        p->context = &Gorgon::Animation::Governor::Active();
                }
            }
            for(auto &wa : s.animations) {
                if(auto a = wa.lock()) {
                    if(a->context == gov)
                        a->context = &Gorgon::Animation::Governor::Active();
                }
            }
            for(auto &wo : s.shortcut_overlays) {
                if(auto o = wo.lock()) {
                    if(o->context == gov)
                        o->context = &Gorgon::Animation::Governor::Active();
                }
            }
            s.context_action_keys.erase(gov);
            s.wake.notify_all();
        }

        void registerGovernorHooks() {
            auto &s = S();
            if(!s.hooks_registered) {
                s.hooks_registered = true;
                Gorgon::Animation::internal::openrgb_governor_activated = &onGovernorActivated;
                Gorgon::Animation::internal::openrgb_governor_destroyed = &onGovernorDestroyed;
            }
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
            d->default_modes = c.modes;

            d->orig_mode = c.active_mode;
            d->hw_mode   = c.active_mode;
            if(c.active_mode >= 0 && c.active_mode < (int)c.modes.size()) {
                d->orig_mode_data = c.modes[c.active_mode];
                d->orig_direct = isDirectMode(d->orig_mode_data) || d->orig_mode_data.name == "Direct";
            }
            d->orig_colors = c.colors;

            seedDefaultModeColors(d->default_modes, d->orig_mode_data, d->orig_colors);
            if(d->orig_mode >= 0 && d->orig_mode < (int)d->default_modes.size()) {
                if((d->orig_mode_data.flags & LL::ModeFlags::HasModeSpecificColor) != 0) {
                    bool any_non_black = false;
                    for(const auto &col : d->orig_mode_data.colors) {
                        if(col.ToWireRGB() != 0) {
                            any_non_black = true;
                            break;
                        }
                    }
                    if(!any_non_black) {
                        d->orig_mode_data = d->default_modes[d->orig_mode];
                    }
                }
            }

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

                if(String::ToLower(led.name).rfind("key:", 0) == 0)
                    keyleds = true;

                d->leds.push_back(led);
                d->initial_base_leds.push_back(led.color);
            }

            d->global_layer = makeDefaultLayer(*d);
            d->is_keyboard  = d->type == DeviceType::Keyboard || keyleds;

            // Registry object
            std::unique_ptr<Device> dev;
            if(d->is_keyboard)
                dev.reset(Access::newKeyboard(d));
            else
                dev.reset(Access::newDevice(d));

            // Zones
            int offset = 0;
            bool has_2d_matrix = false;
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

                    if(!has_2d_matrix) {
                        d->matrix_zone = (int)i;
                        has_2d_matrix = true;
                    }
                }
                else if(lz.leds_count > 0) {
                    // Treat linear / strip zones (and single-LED zones) as width x 1 matrices
                    auto zi = Access::impl(zone);
                    zi->width  = (int)lz.leds_count;
                    zi->height = 1;
                    zi->matrix.resize(zi->width);
                    for(int x = 0; x < zi->width; x++)
                        zi->matrix[x] = x;

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
                            for(int led_idx : it->second) {
                                // Prefer OSTransport scancode keys over 'I' for LED label lookup on Turkish keyboards
                                // where keycode 31 ('ı' / 'I') is mapped to both 'I' and OSTransport+31
                                if(d->led_to_key.find(led_idx) == d->led_to_key.end() || p.first >= K::OSTransport)
                                    d->led_to_key[led_idx] = p.first;
                            }
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
                                d->connected     = true;
                                d->id            = (int)i;
                                d->location      = ctrls[i].location;
                                d->default_modes = ctrls[i].modes;
                                d->hw_mode       = ctrls[i].active_mode;

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

                                // Only snapshot current hardware baseline if we haven't modified this device yet
                                if(!d->hw_touched && resolveActiveLayer(*d) == nullptr) {
                                    d->orig_mode = ctrls[i].active_mode;
                                    if(ctrls[i].active_mode >= 0 && ctrls[i].active_mode < (int)ctrls[i].modes.size()) {
                                        d->orig_mode_data = ctrls[i].modes[ctrls[i].active_mode];
                                        d->orig_direct = isDirectMode(d->orig_mode_data) || d->orig_mode_data.name == "Direct";
                                    }
                                    d->orig_colors = ctrls[i].colors;
                                }
                                seedDefaultModeColors(d->default_modes, d->orig_mode_data, d->orig_colors);
                                if(!d->hw_touched && resolveActiveLayer(*d) == nullptr &&
                                   d->orig_mode >= 0 && d->orig_mode < (int)d->default_modes.size()) {
                                    if((d->orig_mode_data.flags & LL::ModeFlags::HasModeSpecificColor) != 0) {
                                        bool any_non_black = false;
                                        for(const auto &col : d->orig_mode_data.colors) {
                                            if(col.ToWireRGB() != 0) {
                                                any_non_black = true;
                                                break;
                                            }
                                        }
                                        if(!any_non_black) {
                                            d->orig_mode_data = d->default_modes[d->orig_mode];
                                        }
                                    }
                                }

                                if(d->hw_touched || resolveActiveLayer(*d) != nullptr) {
                                    // Hardware reconnection state restoration:
                                    // Software had set colors/modes before disconnect. Re-engage and push state.
                                    d->dirty   = true;
                                    d->engaged = false;
                                    d->hw_mode = -1;
                                    d->last_sent.clear();
                                    d->revision++;
                                    Logger << "Restoring lighting state for reconnected device: " << d->name << " (id " << d->id << ")";
                                }
                                else if(d->orig_direct) {
                                    for(size_t k = 0; k < d->leds.size() && k < ctrls[i].colors.size(); k++) {
                                        auto col = ctrls[i].colors[k];
                                        auto rgba = Graphics::RGBA(col.R(), col.G(), col.B(), Byte(255));
                                        d->leds[k].color = rgba;
                                        if(k < d->global_layer.base_leds.size())
                                            d->global_layer.base_leds[k] = rgba;
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
                            d->id        = -1;
                            d->engaged   = false;
                            d->hw_mode   = -1;
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

        void restoreDeviceHardwareLocked(const std::shared_ptr<DImpl> &d) {
            auto &s = S();
            if(!d->connected || d->id < 0)
                return;

            bool is_direct = d->orig_direct ||
                             (d->orig_mode >= 0 && d->orig_mode == d->direct_mode) ||
                             (d->orig_mode_data.name == "Direct") ||
                             isDirectMode(d->orig_mode_data);

            bool has_per_led = !is_direct &&
                               (((d->orig_mode_data.flags & LL::ModeFlags::HasPerLedColor) != 0) ||
                                (d->orig_mode_data.color_mode == static_cast<uint32_t>(LL::ModeColorMode::PerLed)));

            bool has_mode_specific = !is_direct && !has_per_led &&
                                     (((d->orig_mode_data.flags & LL::ModeFlags::HasModeSpecificColor) != 0) ||
                                      (d->orig_mode_data.color_mode == static_cast<uint32_t>(LL::ModeColorMode::ModeSpecific)));

            if(is_direct) {
                // Direct mode: apply Direct mode, then reinstate original per-LED colors
                s.client.UpdateMode((uint32_t)d->id, d->orig_mode, d->orig_mode_data);
                if(d->orig_colors.size() == d->leds.size()) {
                    s.client.UpdateLEDs((uint32_t)d->id, d->orig_colors);
                }
            }
            else if(has_per_led) {
                // Hardware mode with per-LED colors:
                s.client.UpdateMode((uint32_t)d->id, d->orig_mode, d->orig_mode_data);
                if(d->orig_colors.size() == d->leds.size()) {
                    s.client.UpdateLEDs((uint32_t)d->id, d->orig_colors);
                }
            }
            else if(has_mode_specific) {
                // Hardware effect mode with mode-specific colors (Reactive, Static, Breathing, etc.):
                auto mode_data = d->orig_mode_data;
                if(mode_data.colors.empty() && mode_data.colors_min > 0) {
                    mode_data.colors.resize(mode_data.colors_min, LL::Color(255, 0, 0));
                }
                if(mode_data.colors.empty() && mode_data.colors_max > 0) {
                    mode_data.colors.resize(1, LL::Color(255, 0, 0));
                }
                bool any_non_black = false;
                for(const auto &c : mode_data.colors) {
                    if(c.ToWireRGB() != 0) {
                        any_non_black = true;
                        break;
                    }
                }
                if(!any_non_black && !mode_data.colors.empty()) {
                    bool found_orig = false;
                    for(const auto &c : d->orig_colors) {
                        if(c.ToWireRGB() != 0) {
                            mode_data.colors[0] = c;
                            found_orig = true;
                            break;
                        }
                    }
                    if(!found_orig) {
                        mode_data.colors[0] = LL::Color::FromRGB(0, 200, 255);
                    }
                }
                mode_data.color_mode = static_cast<uint32_t>(LL::ModeColorMode::ModeSpecific);
                s.client.UpdateMode((uint32_t)d->id, d->orig_mode, mode_data);
            }
            else {
                // Other hardware modes (Random, Spectrum Cycle, Rainbow, None):
                s.client.UpdateMode((uint32_t)d->id, d->orig_mode, d->orig_mode_data);
            }
        }

        /// Restores devices modified by software to their snapshot state.
        void restoreLocked() {
            auto &s = S();

            std::vector<std::shared_ptr<DImpl>> list;
            {
                std::lock_guard<std::mutex> sg(s.state_mtx);
                for(auto &d : deviceList()) {
                    if(d->hw_touched && d->connected && d->id >= 0 && s.client.IsConnected()) {
                        list.push_back(d);
                    }
                    else {
                        d->engaged      = false;
                        d->dirty        = false;
                        d->hw_touched   = false;
                        d->hw_mode      = d->orig_mode;
                        d->global_layer = makeDefaultLayer(*d);
                        d->gov_layers.clear();
                        d->last_sent.clear();
                    }
                }
            }

            for(auto &d : list) {
                restoreDeviceHardwareLocked(d);

                std::lock_guard<std::mutex> sg(s.state_mtx);
                d->engaged      = false;
                d->dirty        = false;
                d->hw_touched   = false;
                d->hw_mode      = d->orig_mode;
                d->global_layer = makeDefaultLayer(*d);
                d->gov_layers.clear();
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
                    d->hw_mode   = -1;
                    d->last_sent.clear();
                    if(d->hw_touched || resolveActiveLayer(*d) != nullptr)
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

            if(!s.initialized || s.suspended.load())
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
                uint64_t version = 0;
                bool restore_orig = false;
                bool is_direct = true;
                bool send_mode = false;
                int target_mode = -1;
                LL::Mode mode_data;
                std::vector<LL::Color> colors;
            };

            std::vector<Job> jobs;
            {
                std::lock_guard<std::mutex> sg(s.state_mtx);

                // Clean up expired weak_ptrs
                s.progressbars.erase(
                    std::remove_if(s.progressbars.begin(), s.progressbars.end(),
                                   [](const std::weak_ptr<PImpl> &w) { return w.expired(); }),
                    s.progressbars.end());
                s.animations.erase(
                    std::remove_if(s.animations.begin(), s.animations.end(),
                                   [](const std::weak_ptr<AImpl> &w) { return w.expired(); }),
                    s.animations.end());
                s.shortcut_overlays.erase(
                    std::remove_if(s.shortcut_overlays.begin(), s.shortcut_overlays.end(),
                                   [](const std::weak_ptr<internal::ShortcutOverlay> &w) { return w.expired(); }),
                    s.shortcut_overlays.end());

                const Gorgon::Animation::Governor *active_gov = &Gorgon::Animation::Governor::Active();

                // Check if any active overlay (progressbar, animation, or shortcuts) is active on an otherwise untouched device
                auto hasActiveOverlay = [&](const DImpl &d, const Gorgon::Animation::Governor **overlay_ctx) -> bool {
                    for(auto &wp : s.progressbars) {
                        if(auto p = wp.lock()) {
                            if(p->active && (p->context == active_gov || p->context == nullptr)) {
                                std::vector<std::vector<int>> pos;
                                resolveProgressPositionsForDevice(*p, d, pos);
                                if(!pos.empty()) {
                                    if(overlay_ctx) *overlay_ctx = p->context;
                                    return true;
                                }
                            }
                        }
                    }
                    for(auto &wa : s.animations) {
                        if(auto a = wa.lock()) {
                            if(a->visible && (a->context == active_gov || a->context == nullptr) && animationTargetsDevice(*a, d)) {
                                if(overlay_ctx) *overlay_ctx = a->context;
                                return true;
                            }
                        }
                    }
                    if(d.is_keyboard) {
                        for(auto &wo : s.shortcut_overlays) {
                            if(auto o = wo.lock()) {
                                if(o->active && (o->context == active_gov || o->context == nullptr) && o->mode != ModifierHighlightMode::Disabled && !o->bindings.empty()) {
                                    if(overlay_ctx) *overlay_ctx = o->context;
                                    return true;
                                }
                            }
                        }
                    }
                    return false;
                };

                for(auto &d : deviceList()) {
                    if(!d->dirty || !d->connected || d->id < 0)
                        continue;

                    const Gorgon::Animation::Governor *layer_ctx = nullptr;
                    DeviceLayer *layer = resolveActiveLayer(*d, &layer_ctx);

                    const Gorgon::Animation::Governor *overlay_ctx = nullptr;
                    if(!layer && hasActiveOverlay(*d, &overlay_ctx)) {
                        layer = &getOrCreateLayer(*d, overlay_ctx);
                        if(d->direct_mode >= 0)
                            layer->active_mode = d->direct_mode;
                        layer_ctx = overlay_ctx;
                    }

                    if(!layer) {
                        // Neither active governor nor global layer has touched this device
                        if(d->hw_touched && (d->engaged || d->hw_mode != d->orig_mode)) {
                            Job j;
                            j.d            = d;
                            j.version      = d->revision;
                            j.restore_orig = true;
                            jobs.push_back(std::move(j));
                        }
                        else {
                            d->dirty = false;
                        }
                        continue;
                    }

                    int mode_idx = layer->active_mode;
                    if(mode_idx < 0 || mode_idx >= (int)layer->modes.size())
                        mode_idx = d->direct_mode;

                    bool direct = (mode_idx == d->direct_mode) ||
                                  (mode_idx >= 0 && mode_idx < (int)layer->modes.size() && isDirectMode(layer->modes[mode_idx]));

                    if(direct && d->direct_mode < 0) {
                        d->dirty = false;
                        if(!d->unsupported) {
                            d->unsupported = true;
                            Logger << "Device " << d->name << " does not support direct mode, ignoring";
                        }
                        continue;
                    }

                    Job j;
                    j.d           = d;
                    j.version     = d->revision;
                    j.is_direct   = direct;
                    j.target_mode = mode_idx;

                    if(direct) {
                        j.send_mode = !d->engaged || (d->hw_mode != mode_idx) || layer->mode_dirty;
                        if(j.send_mode && mode_idx >= 0 && mode_idx < (int)layer->modes.size())
                            j.mode_data = layer->modes[mode_idx];
                        j.colors = toWireColors(composeRGBA(*d, *layer, layer_ctx));
                    }
                    else {
                        j.send_mode = d->engaged || (d->hw_mode != mode_idx) || layer->mode_dirty;
                        if(mode_idx >= 0 && mode_idx < (int)layer->modes.size())
                            j.mode_data = layer->modes[mode_idx];
                        if((j.mode_data.flags & LL::ModeFlags::HasPerLedColor) != 0)
                            j.colors = toWireColors(composeRGBA(*d, *layer, layer_ctx));
                    }

                    layer->mode_dirty = false;
                    jobs.push_back(std::move(j));
                }
            }

            for(auto &j : jobs) {
                bool ok = true;

                if(j.restore_orig) {
                    restoreDeviceHardwareLocked(j.d);
                    std::lock_guard<std::mutex> sg(s.state_mtx);
                    j.d->engaged = false;
                    j.d->hw_mode = j.d->orig_mode;
                    j.d->last_sent.clear();
                    if(j.d->revision == j.version)
                        j.d->dirty = false;
                    continue;
                }

                if(!j.is_direct) {
                    if(j.send_mode)
                        ok = s.client.UpdateMode((uint32_t)j.d->id, j.target_mode, j.mode_data);

                    if(!ok)
                        return handleDisconnect(s.client.GetLastError());

                    if((j.mode_data.flags & LL::ModeFlags::HasPerLedColor) != 0 &&
                       j.mode_data.color_mode != static_cast<uint32_t>(LL::ModeColorMode::Random)) {
                        if(j.send_mode || j.colors != j.d->last_sent) {
                            ok = s.client.UpdateLEDs((uint32_t)j.d->id, j.colors);
                            if(!ok)
                                return handleDisconnect(s.client.GetLastError());
                            j.d->last_sent = j.colors;
                        }
                    }
                    else {
                        j.d->last_sent.clear();
                    }

                    std::lock_guard<std::mutex> sg(s.state_mtx);
                    j.d->engaged    = false;
                    j.d->hw_mode    = j.target_mode;
                    j.d->hw_touched = true;
                    if(j.d->revision == j.version)
                        j.d->dirty = false;
                    continue;
                }

                // Direct mode job
                if(j.send_mode) {
                    ok = s.client.UpdateMode((uint32_t)j.d->id, j.target_mode, j.mode_data);
                }
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
                j.d->engaged    = true;
                j.d->hw_mode    = j.target_mode;
                j.d->hw_touched = true;
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

            registerGovernorHooks();

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

        /// Applies a hardware mode in the given governor context
        bool applyMode(const std::shared_ptr<DImpl> &d, const Gorgon::Animation::Governor *ctx, int idx) {
            if(!ensure())
                return false;

            auto &s = S();
            bool apply_now = false;
            {
                std::lock_guard<std::mutex> sg(s.state_mtx);
                if(!d->connected || d->id < 0)
                    return false;

                auto &layer = getOrCreateLayer(*d, ctx);
                if(idx < 0 || idx >= (int)layer.modes.size())
                    return false;

                layer.active_mode = idx;
                layer.touched     = true;
                layer.mode_dirty  = true;
                d->dirty          = true;
                d->revision++;

                const auto &m = layer.modes[idx];
                if((m.flags & LL::ModeFlags::HasPerLedColor) != 0 && !isDirectMode(m)) {
                    bool any_non_black = false;
                    for(const auto &c : layer.base_leds) {
                        if(c.A > 0 && (c.R > 0 || c.G > 0 || c.B > 0)) {
                            any_non_black = true;
                            break;
                        }
                    }
                    if(!any_non_black) {
                        Graphics::RGBA fallback(Byte(255), Byte(255), Byte(0), Byte(255));
                        for(const auto &c : d->orig_colors) {
                            if(c.ToWireRGB() != 0) {
                                fallback = Graphics::RGBA(c.R(), c.G(), c.B(), Byte(255));
                                break;
                            }
                        }
                        if(layer.base_leds.size() < d->leds.size())
                            layer.base_leds.resize(d->leds.size(), fallback);
                        for(auto &c : layer.base_leds)
                            c = fallback;
                        if(!ctx) {
                            for(auto &led : d->leds)
                                led.color = fallback;
                        }
                    }
                }

                const Gorgon::Animation::Governor *eff_ctx = nullptr;
                auto *eff_layer = resolveActiveLayer(*d, &eff_ctx);
                apply_now = (eff_layer == &layer);
            }

            if(s.suspended.load())
                return true;

            if(apply_now) {
                fireError(tick());
            }
            else {
                s.wake.notify_all();
            }

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

        /// state_mtx must be held
        void drawImage(ZImpl &z, const Gorgon::Animation::Governor *ctx, const Containers::Image &img, Geometry::Point offset, BlendMode blend) {
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

                    setLED(*d, ctx, gi, blendColor(getLED(*d, ctx, gi), px, blend));
                }
            }
        }

        void drawScaled(ZImpl &z, const Gorgon::Animation::Governor *ctx, const Containers::Image &img, Containers::InterpolationMethod interp, BlendMode blend) {
            if(z.width == 0 || z.height == 0 || img.GetWidth() == 0 || img.GetHeight() == 0)
                return;

            auto scaled = img.Scale({z.width, z.height}, interp);
            drawImage(z, ctx, scaled, {0, 0}, blend);
        }

        Containers::Image toImage(const ZImpl &z, const Gorgon::Animation::Governor *ctx) {
            auto d = z.dev.lock();
            if(!d || z.width == 0 || z.height == 0)
                return Containers::Image();

            Containers::Image img({z.width, z.height}, Graphics::ColorMode::RGBA);

            for(int y = 0; y < z.height; y++) {
                for(int x = 0; x < z.width; x++) {
                    int local = z.matrix[(size_t)y * z.width + x];

                    Graphics::RGBA c = transparent;
                    if(local >= 0)
                        c = getLED(*d, ctx, z.led_offset + local);

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

    Zone::Zone() : impl(std::make_shared<Impl>()), context(nullptr) { }

    Zone::Zone(int index, const std::string &name, ZoneType type, int led_count, int led_offset, const Device &parent) :
        impl(std::make_shared<Impl>()),
        context(Access::context(parent))
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

        return Device(d, context);
    }

    Graphics::RGBA Zone::GetLEDColor(int local_idx) const {
        auto d = impl->dev.lock();
        if(!d || local_idx < 0 || local_idx >= impl->led_count)
            return transparent;

        std::lock_guard<std::mutex> g(S().state_mtx);
        return getLED(*d, context, impl->led_offset + local_idx);
    }

    void Zone::SetLEDColor(int local_idx, Graphics::RGBA color) {
        auto d = impl->dev.lock();
        if(!d || local_idx < 0 || local_idx >= impl->led_count)
            return;

        std::lock_guard<std::mutex> g(S().state_mtx);
        setLED(*d, context, impl->led_offset + local_idx, color);
    }

    void Zone::SetColor(Graphics::RGBA color) {
        auto d = impl->dev.lock();
        if(!d)
            return;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(int i = 0; i < impl->led_count; i++)
            setLED(*d, context, impl->led_offset + i, color);
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
        drawImage(*impl, context, image, offset, blend);
    }

    void Zone::DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp, BlendMode blend) {
        if(!bitmap.HasData())
            return;

        DrawScaled(bitmap.GetData(), interp, blend);
    }

    void Zone::DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp, BlendMode blend) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        drawScaled(*impl, context, image, interp, blend);
    }

    Containers::Image Zone::ToImage() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        return toImage(*impl, context);
    }

    // -------------------------------------------------------------
    // Device
    // -------------------------------------------------------------

    Device::Device() : impl(std::make_shared<Impl>()), context(nullptr) { }

    Device::Device(std::shared_ptr<Impl> impl, const Gorgon::Animation::Governor *context) :
        impl(std::move(impl)),
        context(context)
    { }

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
        return getLED(*impl, context, led_idx);
    }

    Graphics::RGBA Device::GetLEDColor(const std::string &name) const {
        return GetLEDColor(FindLED(name));
    }

    Graphics::RGBA Device::GetEffectiveLEDColor(int led_idx) const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        if(led_idx < 0 || led_idx >= (int)impl->leds.size())
            return transparent;

        const Gorgon::Animation::Governor *eff_gov = context;
        const DeviceLayer *layer_ptr = nullptr;
        if(context != nullptr) {
            layer_ptr = &getHandleLayer(*impl, context);
        }
        else {
            layer_ptr = resolveActiveLayer(*impl, &eff_gov);
            if(!layer_ptr)
                layer_ptr = &impl->global_layer;
        }

        auto composed = composeRGBA(*impl, *layer_ptr, eff_gov);
        return led_idx < (int)composed.size() ? composed[led_idx] : transparent;
    }

    void Device::SetLEDColor(int led_idx, Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        setLED(*impl, context, led_idx, color);
    }

    void Device::SetLEDColor(const std::string &name, Graphics::RGBA color) {
        SetLEDColor(FindLED(name), color);
    }

    void Device::SetColor(Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        for(int i = 0; i < (int)impl->leds.size(); i++)
            setLED(*impl, context, i, color);
    }

    void Device::SetColors(const std::vector<Graphics::RGBA> &colors) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        for(int i = 0; i < (int)impl->leds.size() && i < (int)colors.size(); i++)
            setLED(*impl, context, i, colors[i]);
    }

    void Device::Clear() {
        SetColor(Graphics::RGBA(Byte(0), Byte(0), Byte(0), Byte(255)));
    }

    int Device::GetZoneCount() const { return (int)impl->zones.size(); }

    Zone &Device::GetZone(int index) {
        auto &z = impl->zones.at(index);
        Access::setImpl(z, Access::impl(z), context);
        return z;
    }

    const Zone &Device::GetZone(int index) const {
        auto &z = const_cast<std::vector<Zone> &>(impl->zones).at(index);
        Access::setImpl(z, Access::impl(z), context);
        return z;
    }

    bool Device::HasZone(const std::string &name) const {
        auto n = String::ToLower(name);
        for(auto &z : impl->zones) {
            if(String::ToLower(z.GetName()) == n)
                return true;
        }

        return false;
    }

    std::optional<Zone> Device::FindZone(const std::string &name) const {
        auto n = String::ToLower(name);
        for(auto &z : impl->zones) {
            if(String::ToLower(z.GetName()) == n)
                return Access::makeZone(z, context);
        }

        for(auto &z : impl->zones) {
            if(String::ToLower(z.GetName()).find(n) != std::string::npos)
                return Access::makeZone(z, context);
        }

        return std::nullopt;
    }

    std::vector<Zone> Device::GetZones() const {
        std::vector<Zone> out;
        out.reserve(impl->zones.size());
        for(const auto &z : impl->zones)
            out.push_back(Access::makeZone(z, context));
        return out;
    }

    std::vector<Zone> Device::GetLinearZones() const {
        std::vector<Zone> out;
        for(const auto &z : impl->zones) {
            if(z.GetType() == ZoneType::Linear)
                out.push_back(Access::makeZone(z, context));
        }
        return out;
    }

    std::vector<Zone> Device::GetSingleZones() const {
        std::vector<Zone> out;
        for(const auto &z : impl->zones) {
            if(z.GetType() == ZoneType::Single)
                out.push_back(Access::makeZone(z, context));
        }
        return out;
    }

    std::vector<Zone> Device::GetMatrixZones() const {
        std::vector<Zone> out;
        for(const auto &z : impl->zones) {
            if(z.GetType() == ZoneType::Matrix)
                out.push_back(Access::makeZone(z, context));
        }
        return out;
    }

    bool Device::HasZoneType(ZoneType type) const {
        for(const auto &z : impl->zones) {
            if(z.GetType() == type)
                return true;
        }
        return false;
    }

    int Device::GetModeCount() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        return (int)getHandleLayer(*impl, context).modes.size();
    }

    std::string Device::GetModeName(Mode mode_index) const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &modes = getHandleLayer(*impl, context).modes;
        if(mode_index < 0 || mode_index >= (int)modes.size())
            return "";

        return modes[mode_index].name;
    }

    std::vector<std::string> Device::GetModeNames() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &modes = getHandleLayer(*impl, context).modes;
        std::vector<std::string> names;
        names.reserve(modes.size());
        for(auto &m : modes)
            names.push_back(m.name);

        return names;
    }

    Device::Mode Device::GetActiveMode() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        return getHandleLayer(*impl, context).active_mode;
    }

    std::string Device::GetActiveModeName() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return "";

        return layer.modes[layer.active_mode].name;
    }

    bool Device::IsDirectMode() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return false;

        return layer.active_mode == impl->direct_mode || isDirectMode(layer.modes[layer.active_mode]);
    }

    void Device::SetMode(Mode mode_index) {
        applyMode(impl, context, mode_index);
    }

    void Device::SetMode(const std::string &mode_name) {
        auto n = String::ToLower(mode_name);
        int idx = -1;
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            const auto &modes = getHandleLayer(*impl, context).modes;
            for(int i = 0; i < (int)modes.size(); i++) {
                if(String::ToLower(modes[i].name) == n) {
                    idx = i;
                    break;
                }
            }
        }
        if(idx >= 0)
            applyMode(impl, context, idx);
    }

    bool Device::SupportsModeColor() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return false;

        const auto &m = layer.modes[layer.active_mode];
        if((m.flags & LL::ModeFlags::HasModeSpecificColor) != 0)
            return true;

        if((m.flags & LL::ModeFlags::HasPerLedColor) != 0 && !isDirectMode(m))
            return true;

        return false;
    }

    int Device::GetModeColorCount() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return 0;

        const auto &m = layer.modes[layer.active_mode];
        if((m.flags & LL::ModeFlags::HasModeSpecificColor) != 0)
            return (int)m.colors.size();

        if((m.flags & LL::ModeFlags::HasPerLedColor) != 0 && !isDirectMode(m))
            return 1;

        return 0;
    }

    int Device::GetModeMinColors() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return 0;

        const auto &m = layer.modes[layer.active_mode];
        if((m.flags & LL::ModeFlags::HasModeSpecificColor) != 0)
            return (int)m.colors_min;

        if((m.flags & LL::ModeFlags::HasPerLedColor) != 0 && !isDirectMode(m))
            return 1;

        return 0;
    }

    int Device::GetModeMaxColors() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return 0;

        const auto &m = layer.modes[layer.active_mode];
        if((m.flags & LL::ModeFlags::HasModeSpecificColor) != 0)
            return (int)m.colors_max;

        if((m.flags & LL::ModeFlags::HasPerLedColor) != 0 && !isDirectMode(m))
            return 1;

        return 0;
    }

    std::vector<Graphics::RGBA> Device::GetModeColors() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        std::vector<Graphics::RGBA> out;
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return out;

        const auto &m = layer.modes[layer.active_mode];
        if((m.flags & LL::ModeFlags::HasModeSpecificColor) != 0) {
            out.reserve(m.colors.size());
            for(const auto &c : m.colors)
                out.emplace_back(c.R(), c.G(), c.B(), Byte(255));
            return out;
        }

        if((m.flags & LL::ModeFlags::HasPerLedColor) != 0 && !isDirectMode(m)) {
            Graphics::RGBA col(Byte(255), Byte(255), Byte(0), Byte(255));
            bool found = false;
            for(const auto &c : layer.base_leds) {
                if(c.A > 0 && (c.R > 0 || c.G > 0 || c.B > 0)) {
                    col = c;
                    found = true;
                    break;
                }
            }
            if(!found) {
                for(const auto &c : impl->orig_colors) {
                    if(c.ToWireRGB() != 0) {
                        col = Graphics::RGBA(c.R(), c.G(), c.B(), Byte(255));
                        found = true;
                        break;
                    }
                }
            }
            out.push_back(col);
            return out;
        }

        return out;
    }

    Graphics::RGBA Device::GetModeColor(int color_index) const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return transparent;

        const auto &m = layer.modes[layer.active_mode];
        if((m.flags & LL::ModeFlags::HasModeSpecificColor) != 0) {
            if(color_index < 0 || color_index >= (int)m.colors.size())
                return transparent;

            const auto &c = m.colors[color_index];
            return Graphics::RGBA(c.R(), c.G(), c.B(), Byte(255));
        }

        if((m.flags & LL::ModeFlags::HasPerLedColor) != 0 && !isDirectMode(m)) {
            if(color_index != 0)
                return transparent;

            for(const auto &c : layer.base_leds) {
                if(c.A > 0 && (c.R > 0 || c.G > 0 || c.B > 0)) {
                    return c;
                }
            }
            for(const auto &c : impl->orig_colors) {
                if(c.ToWireRGB() != 0) {
                    return Graphics::RGBA(c.R(), c.G(), c.B(), Byte(255));
                }
            }
            return Graphics::RGBA(Byte(255), Byte(255), Byte(0), Byte(255));
        }

        return transparent;
    }

    void Device::SetModeColor(Graphics::RGBA color, int color_index) {
        int active = -1;
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            auto &layer = getOrCreateLayer(*impl, context);
            if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
                return;

            auto &m = layer.modes[layer.active_mode];
            if((m.flags & LL::ModeFlags::HasModeSpecificColor) != 0) {
                if(color_index < 0)
                    return;

                int max_cols = m.colors_max > 0 ? (int)m.colors_max : std::max(1, (int)m.colors.size());
                if(color_index >= max_cols)
                    return;

                if(color_index >= (int)m.colors.size())
                    m.colors.resize(color_index + 1, LL::Color(0, 0, 0, 0));

                m.colors[color_index] = wire(color);
                m.color_mode = static_cast<uint32_t>(LL::ModeColorMode::ModeSpecific);
                active = layer.active_mode;
            }
            else if((m.flags & LL::ModeFlags::HasPerLedColor) != 0 && !isDirectMode(m)) {
                if(color_index != 0)
                    return;

                auto col = opaque(color);
                if(layer.base_leds.size() < impl->leds.size())
                    layer.base_leds.resize(impl->leds.size(), col);
                for(auto &c : layer.base_leds)
                    c = col;

                if(!context) {
                    for(auto &led : impl->leds)
                        led.color = col;
                }

                m.color_mode = static_cast<uint32_t>(LL::ModeColorMode::PerLed);
                touchLayer(*impl, layer);
                active = layer.active_mode;
            }
        }

        if(active >= 0)
            applyMode(impl, context, active);
    }

    void Device::SetModeColors(const std::vector<Graphics::RGBA> &colors) {
        int active = -1;
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            auto &layer = getOrCreateLayer(*impl, context);
            if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
                return;

            auto &m = layer.modes[layer.active_mode];
            if((m.flags & LL::ModeFlags::HasModeSpecificColor) != 0) {
                size_t count = colors.size();
                if(m.colors_min > 0 && count < m.colors_min)
                    count = m.colors_min;
                if(m.colors_max > 0 && count > m.colors_max)
                    count = m.colors_max;

                m.colors.resize(count, LL::Color(0, 0, 0, 0));
                for(size_t i = 0; i < count && i < colors.size(); i++)
                    m.colors[i] = wire(colors[i]);

                m.color_mode = static_cast<uint32_t>(LL::ModeColorMode::ModeSpecific);
                active = layer.active_mode;
            }
            else if((m.flags & LL::ModeFlags::HasPerLedColor) != 0 && !isDirectMode(m)) {
                if(colors.empty())
                    return;

                auto col = opaque(colors[0]);
                if(layer.base_leds.size() < impl->leds.size())
                    layer.base_leds.resize(impl->leds.size(), col);
                for(auto &c : layer.base_leds)
                    c = col;

                if(!context) {
                    for(auto &led : impl->leds)
                        led.color = col;
                }

                m.color_mode = static_cast<uint32_t>(LL::ModeColorMode::PerLed);
                touchLayer(*impl, layer);
                active = layer.active_mode;
            }
        }

        if(active >= 0)
            applyMode(impl, context, active);
    }

    bool Device::SupportsRandomColor() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return false;

        return (layer.modes[layer.active_mode].flags & LL::ModeFlags::HasRandomColor) != 0;
    }

    bool Device::IsRandomColor() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return false;

        return layer.modes[layer.active_mode].color_mode == static_cast<uint32_t>(LL::ModeColorMode::Random);
    }

    void Device::SetRandomColor(bool random) {
        int active = -1;
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            auto &layer = getOrCreateLayer(*impl, context);
            if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
                return;

            auto &m = layer.modes[layer.active_mode];
            if(random && (m.flags & LL::ModeFlags::HasRandomColor) != 0) {
                m.color_mode = static_cast<uint32_t>(LL::ModeColorMode::Random);
                active = layer.active_mode;
            }
            else if(!random && (m.flags & LL::ModeFlags::HasModeSpecificColor) != 0) {
                m.color_mode = static_cast<uint32_t>(LL::ModeColorMode::ModeSpecific);
                active = layer.active_mode;
            }
            else if(!random && (m.flags & LL::ModeFlags::HasPerLedColor) != 0) {
                m.color_mode = static_cast<uint32_t>(LL::ModeColorMode::PerLed);
                active = layer.active_mode;
            }
        }

        if(active >= 0)
            applyMode(impl, context, active);
    }

    bool Device::SupportsModeSpeed() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return false;
        return (layer.modes[layer.active_mode].flags & LL::ModeFlags::HasSpeed) != 0;
    }

    unsigned Device::GetModeSpeed() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return 0;
        return layer.modes[layer.active_mode].speed;
    }

    unsigned Device::GetModeMinSpeed() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return 0;
        return layer.modes[layer.active_mode].speed_min;
    }

    unsigned Device::GetModeMaxSpeed() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return 0;
        return layer.modes[layer.active_mode].speed_max;
    }

    void Device::SetModeSpeed(unsigned speed) {
        int active = -1;
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            auto &layer = getOrCreateLayer(*impl, context);
            if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
                return;
            auto &m = layer.modes[layer.active_mode];
            if((m.flags & LL::ModeFlags::HasSpeed) == 0)
                return;
            if(m.speed_max > 0 || m.speed_min > 0) {
                auto [low, high] = std::minmax(m.speed_min, m.speed_max);
                speed = std::clamp(speed, low, high);
            }
            m.speed = speed;
            active = layer.active_mode;
        }
        if(active >= 0)
            applyMode(impl, context, active);
    }

    bool Device::SupportsModeDirection() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return false;
        uint32_t dir_mask = LL::ModeFlags::HasDirectionLR | LL::ModeFlags::HasDirectionUD | LL::ModeFlags::HasDirectionHV;
        return (layer.modes[layer.active_mode].flags & dir_mask) != 0;
    }

    bool Device::SupportsModeDirectionLR() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return false;
        return (layer.modes[layer.active_mode].flags & LL::ModeFlags::HasDirectionLR) != 0;
    }

    bool Device::SupportsModeDirectionUD() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return false;
        return (layer.modes[layer.active_mode].flags & LL::ModeFlags::HasDirectionUD) != 0;
    }

    bool Device::SupportsModeDirectionHV() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return false;
        return (layer.modes[layer.active_mode].flags & LL::ModeFlags::HasDirectionHV) != 0;
    }

    ModeDirection Device::GetModeDirection() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return ModeDirection::Left;
        return static_cast<ModeDirection>(layer.modes[layer.active_mode].direction);
    }

    void Device::SetModeDirection(ModeDirection direction) {
        int active = -1;
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            auto &layer = getOrCreateLayer(*impl, context);
            if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
                return;
            auto &m = layer.modes[layer.active_mode];
            uint32_t dir_mask = LL::ModeFlags::HasDirectionLR | LL::ModeFlags::HasDirectionUD | LL::ModeFlags::HasDirectionHV;
            if((m.flags & dir_mask) == 0)
                return;
            m.direction = static_cast<uint32_t>(direction);
            active = layer.active_mode;
        }
        if(active >= 0)
            applyMode(impl, context, active);
    }

    bool Device::SupportsModeBrightness() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return false;
        return (layer.modes[layer.active_mode].flags & LL::ModeFlags::HasBrightness) != 0;
    }

    unsigned Device::GetModeBrightness() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return 0;
        return layer.modes[layer.active_mode].brightness;
    }

    unsigned Device::GetModeMinBrightness() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return 0;
        return layer.modes[layer.active_mode].brightness_min;
    }

    unsigned Device::GetModeMaxBrightness() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        const auto &layer = getHandleLayer(*impl, context);
        if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
            return 0;
        return layer.modes[layer.active_mode].brightness_max;
    }

    void Device::SetModeBrightness(unsigned brightness) {
        int active = -1;
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            auto &layer = getOrCreateLayer(*impl, context);
            if(layer.active_mode < 0 || layer.active_mode >= (int)layer.modes.size())
                return;
            auto &m = layer.modes[layer.active_mode];
            if((m.flags & LL::ModeFlags::HasBrightness) == 0)
                return;
            if(m.brightness_max > 0 || m.brightness_min > 0) {
                auto [low, high] = std::minmax(m.brightness_min, m.brightness_max);
                brightness = std::clamp(brightness, low, high);
            }
            m.brightness = brightness;
            active = layer.active_mode;
        }
        if(active >= 0)
            applyMode(impl, context, active);
    }

    bool Device::SetDirectMode() {
        if(impl->direct_mode < 0)
            return false;

        return applyMode(impl, context, impl->direct_mode);
    }

    std::optional<Zone> Device::GetMatrixZone() const {
        if(impl->matrix_zone < 0 || impl->matrix_zone >= (int)impl->zones.size())
            return std::nullopt;

        return Access::makeZone(impl->zones[impl->matrix_zone], context);
    }

    Geometry::Size Device::GetMatrixSize() const {
        auto z = GetMatrixZone();
        return z ? z->GetMatrixSize() : Geometry::Size(0, 0);
    }

    int Device::GetWidth() const {
        auto z = GetMatrixZone();
        return z ? z->GetWidth() : 0;
    }

    int Device::GetHeight() const {
        auto z = GetMatrixZone();
        return z ? z->GetHeight() : 0;
    }

    Graphics::RGBA Device::GetPixel(int x, int y) const {
        auto z = GetMatrixZone();
        return z ? z->GetPixel(x, y) : transparent;
    }

    Graphics::RGBA Device::GetPixel(Geometry::Point p) const {
        return GetPixel(p.X, p.Y);
    }

    void Device::SetPixel(int x, int y, Graphics::RGBA color) {
        if(auto z = GetMatrixZone())
            z->SetPixel(x, y, color);
    }

    void Device::SetPixel(Geometry::Point p, Graphics::RGBA color) {
        SetPixel(p.X, p.Y, color);
    }

    void Device::Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset, BlendMode blend) {
        if(auto z = GetMatrixZone())
            z->Draw(bitmap, offset, blend);
    }

    void Device::Draw(const Containers::Image &image, Geometry::Point offset, BlendMode blend) {
        if(auto z = GetMatrixZone())
            z->Draw(image, offset, blend);
    }

    void Device::DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp, BlendMode blend) {
        if(auto z = GetMatrixZone())
            z->DrawScaled(bitmap, interp, blend);
    }

    void Device::DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp, BlendMode blend) {
        if(auto z = GetMatrixZone())
            z->DrawScaled(image, interp, blend);
    }

    Containers::Image Device::ToImage() const {
        auto z = GetMatrixZone();
        return z ? z->ToImage() : Containers::Image();
    }

    // -------------------------------------------------------------
    // KeyboardDevice
    // -------------------------------------------------------------

    KeyboardDevice::KeyboardDevice(std::shared_ptr<Impl> impl, const Gorgon::Animation::Governor *context) :
        Device(std::move(impl), context)
    { }

    void KeyboardDevice::SetKeyColor(Input::Key key, Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);

        auto it = impl->keymap.find(key);
        if(it == impl->keymap.end())
            return;

        for(int led : it->second)
            setLED(*impl, context, led, color);
    }

    void KeyboardDevice::SetKeyColor(const std::string &key_name, Graphics::RGBA color) {
        SetLEDColor(FindLED(key_name), color);
    }

    Graphics::RGBA KeyboardDevice::GetKeyColor(Input::Key key) const {
        std::lock_guard<std::mutex> g(S().state_mtx);

        auto it = impl->keymap.find(key);
        if(it == impl->keymap.end() || it->second.empty())
            return transparent;

        return getLED(*impl, context, it->second[0]);
    }

    void KeyboardDevice::SetKeyGroupColor(const KeyGroup &group, Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);

        for(auto key : group.keys) {
            auto it = impl->keymap.find(key);
            if(it == impl->keymap.end())
                continue;

            for(int led : it->second)
                setLED(*impl, context, led, color);
        }
    }

    bool KeyboardDevice::HasKey(Input::Key key) const {
        return impl->keymap.count(key) != 0;
    }

    bool KeyboardDevice::HasKey(const std::string &key_name) const {
        return FindLED(key_name) >= 0;
    }

    std::vector<Input::Key> KeyboardDevice::GetKeys() const {
        std::vector<Input::Key> keys;
        keys.reserve(impl->keymap.size());
        for(const auto &pair : impl->keymap)
            keys.push_back(pair.first);
        return keys;
    }

    std::vector<int> KeyboardDevice::GetKeyLEDIndices(Input::Key key) const {
        auto it = impl->keymap.find(key);
        if(it == impl->keymap.end())
            return {};

        return it->second;
    }

    std::optional<Geometry::Point> KeyboardDevice::GetKeyMatrixPosition(Input::Key key) const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        return findKeyMatrixCoord(*impl, key);
    }

    Input::Key KeyboardDevice::GetKeyForLED(int led_index) const {
        auto it = impl->led_to_key.find(led_index);
        if(it == impl->led_to_key.end())
            return 0;

        return it->second;
    }

    std::string KeyboardDevice::GetKeyLabel(int led_index) const {
        if(led_index < 0 || led_index >= (int)impl->leds.size())
            return "";

        auto it = impl->led_to_key.find(led_index);
        if(it != impl->led_to_key.end()) {
            std::string os_name = Input::Keyboard::Keycodes::GetName(it->second);
            // Ignore replacement char U+FFFD (UTF-8: \xEF\xBF\xBD) or empty strings
            if(!os_name.empty() && os_name != "\xEF\xBF\xBD")
                return os_name;
        }

        const std::string &raw = impl->leds[led_index].name;
        if(raw.rfind("Key: ", 0) == 0)
            return raw.substr(5);

        return raw;
    }

    // -------------------------------------------------------------
    // Progressbar
    // -------------------------------------------------------------

    namespace {
        void registerProgressbar(const std::shared_ptr<PImpl> &p) {
            auto &s = S();
            std::lock_guard<std::mutex> g(s.state_mtx);
            s.progressbars.push_back(p);
        }

        void markProgressDirtyLocked(const PImpl &) {
            markAllDevicesDirtyLocked();
        }
    }

    Progressbar::Progressbar() : impl(std::make_shared<Impl>()) {
        registerProgressbar(impl);
    }

    Progressbar::Progressbar(const KeyGroup &keys, Graphics::RGBA color, Graphics::RGBA bg) : Progressbar() {
        impl->target   = Impl::Target::AllKeyboards;
        impl->keys     = keys;
        impl->use_keys = true;
        impl->color    = color;
        impl->bg       = bg;
    }

    Progressbar::Progressbar(const KeyGroup &keys, const KeyboardDevice &keyboard, Graphics::RGBA color, Graphics::RGBA bg) : Progressbar() {
        impl->context  = Access::context(keyboard);
        impl->target   = Impl::Target::Keyboard;
        impl->device   = Access::impl(keyboard);
        impl->keys     = keys;
        impl->use_keys = true;
        impl->color    = color;
        impl->bg       = bg;
    }

    Progressbar::Progressbar(const KeyGroup &keys, const Zone &zone, Graphics::RGBA color, Graphics::RGBA bg) : Progressbar() {
        impl->context  = Access::context(zone);
        impl->target   = Impl::Target::Zone;
        impl->zone     = Access::impl(zone);
        impl->keys     = keys;
        impl->use_keys = true;
        impl->color    = color;
        impl->bg       = bg;
    }

    Progressbar::Progressbar(const Zone &zone, Graphics::RGBA color, Graphics::RGBA bg) : Progressbar() {
        impl->context  = Access::context(zone);
        impl->target   = Impl::Target::Zone;
        impl->zone     = Access::impl(zone);
        impl->use_keys = false;
        impl->color    = color;
        impl->bg       = bg;
    }

    Progressbar::Progressbar(const Device &device, Graphics::RGBA color, Graphics::RGBA bg) : Progressbar() {
        impl->context  = Access::context(device);
        impl->target   = Impl::Target::Device;
        impl->device   = Access::impl(device);
        impl->use_keys = false;
        impl->color    = color;
        impl->bg       = bg;
    }

    const Gorgon::Animation::Governor *Progressbar::GetGovernor() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        return impl->context;
    }

    void Progressbar::SetKeys(const KeyGroup &keys) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->keys     = keys;
        impl->use_keys = true;
        if(impl->active)
            markProgressDirtyLocked(*impl);
    }

    const KeyGroup &Progressbar::GetKeys() const { return impl->keys; }

    void Progressbar::SetZone(const Zone &zone) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->target = Impl::Target::Zone;
        impl->zone   = Access::impl(zone);
        if(Access::context(zone))
            impl->context = Access::context(zone);
        if(impl->active)
            markProgressDirtyLocked(*impl);
    }

    void Progressbar::SetDevice(const Device &device) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->target   = Impl::Target::Device;
        impl->device   = Access::impl(device);
        impl->use_keys = false;
        if(Access::context(device))
            impl->context = Access::context(device);
        if(impl->active)
            markProgressDirtyLocked(*impl);
    }

    void Progressbar::TargetAllStrips() {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->target   = Impl::Target::AllStrips;
        impl->use_keys = false;
        if(impl->active)
            markProgressDirtyLocked(*impl);
    }

    void Progressbar::SetColor(Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->color = color;
        if(impl->active)
            markProgressDirtyLocked(*impl);
    }

    Graphics::RGBA Progressbar::GetColor() const { return impl->color; }

    void Progressbar::SetBackgroundColor(Graphics::RGBA bg) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->bg = bg;
        if(impl->active)
            markProgressDirtyLocked(*impl);
    }

    Graphics::RGBA Progressbar::GetBackgroundColor() const { return impl->bg; }

    void Progressbar::AddLevelColor(Range<Float> range, Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->levels.push_back({range, color});
        if(impl->active)
            markProgressDirtyLocked(*impl);
    }

    void Progressbar::SetLevelColors(const std::vector<LevelColor> &levels) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->levels = levels;
        if(impl->active)
            markProgressDirtyLocked(*impl);
    }

    void Progressbar::ClearLevelColors() {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->levels.clear();
        if(impl->active)
            markProgressDirtyLocked(*impl);
    }

    const std::vector<Progressbar::LevelColor> &Progressbar::GetLevelColors() const { return impl->levels; }

    void Progressbar::SetFillMode(bool fill) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->fill = fill;
        if(impl->active)
            markProgressDirtyLocked(*impl);
    }

    bool Progressbar::IsFillMode() const { return impl->fill; }

    void Progressbar::SetProgressRatio(Float value) {
        ensure();

        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->ratio  = Clamp(value, Float(0), Float(1));
        impl->active = true;
        markProgressDirtyLocked(*impl);
    }

    Float Progressbar::GetProgressRatio() const { return impl->ratio; }

    void Progressbar::Clear() {
        std::lock_guard<std::mutex> g(S().state_mtx);
        if(impl->active) {
            impl->active = false;
            markProgressDirtyLocked(*impl);
        }
    }

    // -------------------------------------------------------------
    // OpenRGB::Animation & Derived Effects
    // -------------------------------------------------------------

    namespace {
        std::shared_ptr<AImpl> makeAnimImpl(Gorgon::Animation::Governor *gov, unsigned duration_ms, BlendMode blend) {
            auto a = std::make_shared<AImpl>();
            a->context     = gov;
            a->duration_ms = std::max(1u, duration_ms);
            a->blend       = blend;
            a->owned_ctrl  = std::make_unique<Gorgon::Animation::Controller>(0.0);
            if(gov)
                a->owned_ctrl->SetGovernor(*gov);
            a->owned_ctrl->SetLength(a->duration_ms);
            a->owned_ctrl->Pause();

            {
                std::lock_guard<std::mutex> g(S().state_mtx);
                S().animations.push_back(a);
            }
            return a;
        }
    }

    Animation::Animation(std::shared_ptr<Impl> impl_ptr) :
        Gorgon::Animation::Base(*impl_ptr->owned_ctrl),
        FinishedEvent(impl_ptr->owned_ctrl->FinishedEvent),
        impl(std::move(impl_ptr))
    { }

    Animation::Animation(unsigned duration_ms, BlendMode blend) :
        Animation(makeAnimImpl(&Gorgon::Animation::Governor::Active(), duration_ms, blend))
    { }

    Animation::Animation(Gorgon::Animation::Governor &governor, unsigned duration_ms, BlendMode blend) :
        Animation(makeAnimImpl(&governor, duration_ms, blend))
    { }

    Animation::Animation(const Context &context, unsigned duration_ms, BlendMode blend) :
        Animation(makeAnimImpl(
            context.GetGovernor() ? const_cast<Gorgon::Animation::Governor *>(context.GetGovernor()) : &Gorgon::Animation::Governor::Active(),
            duration_ms, blend))
    {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->context = context.GetGovernor();
    }

    Animation::Animation(const Animation &other) :
        Gorgon::Animation::Base(other),
        FinishedEvent(other.impl->owned_ctrl->FinishedEvent),
        impl(other.impl)
    { }

    Animation &Animation::operator=(const Animation &other) {
        if(this != &other) {
            RemoveController();
            impl = other.impl;
            if(impl && impl->owned_ctrl)
                SetController(*impl->owned_ctrl);
        }
        return *this;
    }

    Animation::~Animation() {
        RemoveController();
        if(impl && impl.use_count() == 1) {
            std::lock_guard<std::mutex> g(S().state_mtx);
            if(impl->visible) {
                impl->visible = false;
                markAllDevicesDirtyLocked();
            }
        }
    }

    void Animation::TargetAllDevices() {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->target   = Impl::Target::AllDevices;
        impl->use_keys = false;
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void Animation::TargetAllKeyboards() {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->target   = Impl::Target::AllKeyboards;
        impl->use_keys = false;
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void Animation::TargetKeyGroup(const KeyGroup &group) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        if(impl->target != Impl::Target::Keyboard && impl->target != Impl::Target::Zone)
            impl->target = Impl::Target::AllKeyboards;
        impl->keys     = group;
        impl->use_keys = true;
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void Animation::TargetKey(Input::Key key) {
        TargetKeyGroup(KeyGroup{{key}});
    }

    void Animation::TargetAllStrips() {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->target   = Impl::Target::AllStrips;
        impl->use_keys = false;
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void Animation::TargetDevice(const Device &device) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->target   = Impl::Target::Device;
        impl->device   = Access::impl(device);
        impl->use_keys = false;
        if(Access::context(device))
            impl->context = Access::context(device);
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void Animation::TargetKeyboard(const KeyboardDevice &keyboard) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->target   = Impl::Target::Keyboard;
        impl->device   = Access::impl(keyboard);
        impl->use_keys = false;
        if(Access::context(keyboard))
            impl->context = Access::context(keyboard);
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void Animation::TargetKeyboardKeys(const KeyboardDevice &keyboard, const KeyGroup &group) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->target   = Impl::Target::Keyboard;
        impl->device   = Access::impl(keyboard);
        impl->keys     = group;
        impl->use_keys = true;
        if(Access::context(keyboard))
            impl->context = Access::context(keyboard);
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void Animation::TargetKeyboardKey(const KeyboardDevice &keyboard, Input::Key key) {
        TargetKeyboardKeys(keyboard, KeyGroup{{key}});
    }

    void Animation::TargetZone(const Zone &zone) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->target = Impl::Target::Zone;
        impl->zone   = Access::impl(zone);
        if(Access::context(zone))
            impl->context = Access::context(zone);
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void Animation::Play() {
        ensure();
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            impl->visible             = true;
            impl->current_progress_ms = 0;
            markAllDevicesDirtyLocked();
        }
        impl->owned_ctrl->Reset();
        impl->owned_ctrl->SetLength(impl->duration_ms);
        impl->owned_ctrl->Play();
    }

    void Animation::Loop() {
        ensure();
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            impl->visible = true;
            markAllDevicesDirtyLocked();
        }
        impl->owned_ctrl->SetLength(impl->duration_ms);
        impl->owned_ctrl->Loop();
    }

    void Animation::Pause() {
        impl->owned_ctrl->Pause();
    }

    void Animation::Resume() {
        ensure();
        {
            std::lock_guard<std::mutex> g(S().state_mtx);
            impl->visible = true;
            markAllDevicesDirtyLocked();
        }
        impl->owned_ctrl->Resume();
    }

    void Animation::Reset() {
        impl->owned_ctrl->Reset();
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->current_progress_ms = 0;
        if(impl->visible)
            markAllDevicesDirtyLocked();
    }

    void Animation::Stop() {
        impl->owned_ctrl->Pause();
        impl->owned_ctrl->SetProgress(0u);
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->active_presses.clear();
        if(impl->visible) {
            impl->visible             = false;
            impl->current_progress_ms = 0;
            markAllDevicesDirtyLocked();
        }
    }

    bool Animation::IsPlaying() const {
        return impl->owned_ctrl->IsPlaying() && impl->visible;
    }

    bool Animation::IsPaused() const {
        return impl->owned_ctrl->IsPaused();
    }

    bool Animation::IsFinished() const {
        return impl->owned_ctrl->IsFinished();
    }

    void Animation::SetSpeed(float speed) {
        impl->owned_ctrl->SetSpeed(speed);
    }

    float Animation::GetSpeed() const {
        return impl->owned_ctrl->GetSpeed();
    }

    void Animation::SetDuration(unsigned duration_ms) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->duration_ms = std::max(1u, duration_ms);
        impl->owned_ctrl->SetLength(impl->duration_ms);
    }

    int Animation::GetDuration() const {
        return (int)impl->duration_ms;
    }

    void Animation::SetBlendMode(BlendMode blend) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->blend = blend;
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    BlendMode Animation::GetBlendMode() const {
        return impl->blend;
    }

    void Animation::SetKeepWhenFinished(bool keep) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->keep_when_finished = keep;
    }

    bool Animation::GetKeepWhenFinished() const {
        return impl->keep_when_finished;
    }

    Gorgon::Animation::Controller &Animation::GetAnimationController() {
        return *impl->owned_ctrl;
    }

    const Gorgon::Animation::Controller &Animation::GetAnimationController() const {
        return *impl->owned_ctrl;
    }

    const Gorgon::Animation::Governor *Animation::GetGovernor() const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        return impl->context;
    }

    void Animation::SetGovernor(Gorgon::Animation::Governor &governor) {
        impl->owned_ctrl->SetGovernor(governor);
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->context = &governor;
        markAllDevicesDirtyLocked();
    }

    bool Animation::Progress(unsigned &leftover) {
        leftover = 0;
        if(!controller)
            return false;

        unsigned prog = controller->GetProgress();
        std::lock_guard<std::mutex> g(S().state_mtx);

        if(impl->type == Impl::Type::Reactive) {
            impl->reactive_clock_ms = prog;
            unsigned dur = impl->duration_ms;
            auto before = impl->active_presses.size();
            impl->active_presses.erase(
                std::remove_if(impl->active_presses.begin(), impl->active_presses.end(),
                               [&](const Impl::KeyPressState &kp) {
                                   return prog >= kp.start_ms && (prog - kp.start_ms) >= dur;
                               }),
                impl->active_presses.end());

            if(!impl->active_presses.empty() || before > 0)
                markAllDevicesDirtyLocked();

            return true; // Reactive effect runs continuously while active
        }

        if(!impl->visible)
            return true;

        if(controller->IsControlled()) {
            if(prog > impl->duration_ms) {
                impl->current_progress_ms = impl->duration_ms;
                leftover = prog - impl->duration_ms;
                if(!impl->owned_ctrl->IsLooping() && !impl->keep_when_finished)
                    impl->visible = false;
                markAllDevicesDirtyLocked();
                return false;
            }
            if(impl->current_progress_ms != prog) {
                impl->current_progress_ms = prog;
                markAllDevicesDirtyLocked();
            }
            return true;
        }

        unsigned wrapped = prog % std::max(1u, impl->duration_ms);
        if(impl->current_progress_ms != wrapped) {
            impl->current_progress_ms = wrapped;
            markAllDevicesDirtyLocked();
        }
        return true;
    }

    // --- FadeEffect ---

    FadeEffect::FadeEffect(Graphics::RGBA start_color, Graphics::RGBA end_color, unsigned duration_ms, BlendMode blend) :
        Animation(duration_ms, blend)
    {
        impl->type  = Impl::Type::Fade;
        impl->stops = {start_color, end_color};
    }

    FadeEffect::FadeEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA start_color, Graphics::RGBA end_color, unsigned duration_ms, BlendMode blend) :
        Animation(governor, duration_ms, blend)
    {
        impl->type  = Impl::Type::Fade;
        impl->stops = {start_color, end_color};
    }

    FadeEffect::FadeEffect(const Context &context, Graphics::RGBA start_color, Graphics::RGBA end_color, unsigned duration_ms, BlendMode blend) :
        Animation(context, duration_ms, blend)
    {
        impl->type  = Impl::Type::Fade;
        impl->stops = {start_color, end_color};
    }

    void FadeEffect::SetColors(Graphics::RGBA start_color, Graphics::RGBA end_color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->stops = {start_color, end_color};
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void FadeEffect::SetColorStops(const std::vector<Graphics::RGBA> &stops) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->stops = stops;
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    // --- PulseEffect ---

    PulseEffect::PulseEffect(Graphics::RGBA peak_color, unsigned period_ms, Graphics::RGBA trough_color, BlendMode blend) :
        Animation(period_ms, blend)
    {
        impl->type  = Impl::Type::Pulse;
        impl->stops = {peak_color, trough_color};
    }

    PulseEffect::PulseEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA peak_color, unsigned period_ms, Graphics::RGBA trough_color, BlendMode blend) :
        Animation(governor, period_ms, blend)
    {
        impl->type  = Impl::Type::Pulse;
        impl->stops = {peak_color, trough_color};
    }

    PulseEffect::PulseEffect(const Context &context, Graphics::RGBA peak_color, unsigned period_ms, Graphics::RGBA trough_color, BlendMode blend) :
        Animation(context, period_ms, blend)
    {
        impl->type  = Impl::Type::Pulse;
        impl->stops = {peak_color, trough_color};
    }

    void PulseEffect::SetColors(Graphics::RGBA peak_color, Graphics::RGBA trough_color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->stops = {peak_color, trough_color};
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    // --- BlinkEffect ---

    BlinkEffect::BlinkEffect(Graphics::RGBA on_color, unsigned period_ms, float duty_cycle, Graphics::RGBA off_color, BlendMode blend) :
        Animation(period_ms, blend)
    {
        impl->type       = Impl::Type::Blink;
        impl->stops      = {on_color, off_color};
        impl->duty_cycle = Clamp(duty_cycle, 0.05f, 0.95f);
    }

    BlinkEffect::BlinkEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA on_color, unsigned period_ms, float duty_cycle, Graphics::RGBA off_color, BlendMode blend) :
        Animation(governor, period_ms, blend)
    {
        impl->type       = Impl::Type::Blink;
        impl->stops      = {on_color, off_color};
        impl->duty_cycle = Clamp(duty_cycle, 0.05f, 0.95f);
    }

    BlinkEffect::BlinkEffect(const Context &context, Graphics::RGBA on_color, unsigned period_ms, float duty_cycle, Graphics::RGBA off_color, BlendMode blend) :
        Animation(context, period_ms, blend)
    {
        impl->type       = Impl::Type::Blink;
        impl->stops      = {on_color, off_color};
        impl->duty_cycle = Clamp(duty_cycle, 0.05f, 0.95f);
    }

    void BlinkEffect::SetColors(Graphics::RGBA on_color, Graphics::RGBA off_color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->stops = {on_color, off_color};
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void BlinkEffect::SetDutyCycle(float duty_cycle) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->duty_cycle = Clamp(duty_cycle, 0.05f, 0.95f);
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    // --- ExplosionEffect ---

    ExplosionEffect::ExplosionEffect(Graphics::RGBA color, unsigned duration_ms, float ring_thickness, BlendMode blend) :
        Animation(duration_ms, blend)
    {
        impl->type           = Impl::Type::Explosion;
        impl->target         = Impl::Target::AllDevices;
        impl->stops          = {color};
        impl->ring_thickness = ring_thickness;
    }

    ExplosionEffect::ExplosionEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA color, unsigned duration_ms, float ring_thickness, BlendMode blend) :
        Animation(governor, duration_ms, blend)
    {
        impl->type           = Impl::Type::Explosion;
        impl->target         = Impl::Target::AllDevices;
        impl->stops          = {color};
        impl->ring_thickness = ring_thickness;
    }

    ExplosionEffect::ExplosionEffect(const Context &context, Graphics::RGBA color, unsigned duration_ms, float ring_thickness, BlendMode blend) :
        Animation(context, duration_ms, blend)
    {
        impl->type           = Impl::Type::Explosion;
        impl->target         = Impl::Target::AllDevices;
        impl->stops          = {color};
        impl->ring_thickness = ring_thickness;
    }

    void ExplosionEffect::SetOriginKey(Input::Key key) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->origin_mode = Impl::OriginMode::Key;
        impl->origin_key  = key;
    }

    void ExplosionEffect::SetOriginPoint(Geometry::Point matrix_coord) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->origin_mode = Impl::OriginMode::Point;
        impl->origin_pt   = matrix_coord;
    }

    void ExplosionEffect::SetOriginNormalized(Geometry::Pointf norm_coord) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->origin_mode = Impl::OriginMode::Normalized;
        impl->origin_norm = norm_coord;
    }

    void ExplosionEffect::SetColor(Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->stops = {color};
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void ExplosionEffect::SetRingThickness(float thickness) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->ring_thickness = thickness;
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void ExplosionEffect::SetMaxRadius(float max_radius) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->max_radius = max_radius;
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    // --- SwipeEffect ---

    SwipeEffect::SwipeEffect(Graphics::RGBA color, SwipeDirection direction, unsigned duration_ms, float band_width, BlendMode blend) :
        Animation(duration_ms, blend)
    {
        impl->type       = Impl::Type::Swipe;
        impl->target     = Impl::Target::AllDevices;
        impl->stops      = {color};
        impl->swipe_dir  = direction;
        impl->band_width = band_width;
    }

    SwipeEffect::SwipeEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA color, SwipeDirection direction, unsigned duration_ms, float band_width, BlendMode blend) :
        Animation(governor, duration_ms, blend)
    {
        impl->type       = Impl::Type::Swipe;
        impl->target     = Impl::Target::AllDevices;
        impl->stops      = {color};
        impl->swipe_dir  = direction;
        impl->band_width = band_width;
    }

    SwipeEffect::SwipeEffect(const Context &context, Graphics::RGBA color, SwipeDirection direction, unsigned duration_ms, float band_width, BlendMode blend) :
        Animation(context, duration_ms, blend)
    {
        impl->type       = Impl::Type::Swipe;
        impl->target     = Impl::Target::AllDevices;
        impl->stops      = {color};
        impl->swipe_dir  = direction;
        impl->band_width = band_width;
    }

    void SwipeEffect::SetColor(Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->stops = {color};
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void SwipeEffect::SetDirection(SwipeDirection direction) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->swipe_dir = direction;
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    void SwipeEffect::SetBandWidth(float width_ratio) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->band_width = Clamp(width_ratio, 0.05f, 2.f);
        if(impl->visible) markAllDevicesDirtyLocked();
    }

    // --- ReactiveEffect ---

    ReactiveEffect::ReactiveEffect(Graphics::RGBA color, unsigned fade_ms, BlendMode blend) :
        Animation(fade_ms, blend)
    {
        impl->type   = Impl::Type::Reactive;
        impl->target = Impl::Target::AllDevices;
        impl->stops  = {color};
        Loop();
    }

    ReactiveEffect::ReactiveEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA color, unsigned fade_ms, BlendMode blend) :
        Animation(governor, fade_ms, blend)
    {
        impl->type   = Impl::Type::Reactive;
        impl->target = Impl::Target::AllDevices;
        impl->stops  = {color};
        Loop();
    }

    ReactiveEffect::ReactiveEffect(const Context &context, Graphics::RGBA color, unsigned fade_ms, BlendMode blend) :
        Animation(context, fade_ms, blend)
    {
        impl->type   = Impl::Type::Reactive;
        impl->target = Impl::Target::AllDevices;
        impl->stops  = {color};
        Loop();
    }

    namespace {
        void triggerReactiveKey(AImpl &a, Input::Key key, bool force_start) {
            ensure();
            std::lock_guard<std::mutex> g(S().state_mtx);
            if(!a.visible) {
                if(!force_start)
                    return;
                a.visible = true;
            }

            Graphics::RGBA col = a.stops.empty() ? Graphics::RGBA(0xffffffffu) : a.stops[0];
            if(a.random_reactive) {
                static const Graphics::RGBA palette[] = {
                    {255, 40,  40,  255},
                    {255, 160, 0,   255},
                    {255, 240, 0,   255},
                    {0,   240, 80,  255},
                    {0,   220, 255, 255},
                    {80,  100, 255, 255},
                    {200, 40,  255, 255},
                    {255, 40,  160, 255}
                };
                a.rng_state = a.rng_state * 1664525u + 1013904223u;
                col = palette[(a.rng_state >> 16) % 8];
            }

            for(auto &kp : a.active_presses) {
                if(kp.key == key) {
                    kp.start_ms = a.reactive_clock_ms;
                    kp.color    = col;
                    markAllDevicesDirtyLocked();
                    return;
                }
            }

            a.active_presses.push_back({key, a.reactive_clock_ms, col});
            markAllDevicesDirtyLocked();
        }
    }

    void ReactiveEffect::TriggerKey(Input::Key key) {
        if(!impl->owned_ctrl->IsPlaying())
            Loop();
        triggerReactiveKey(*impl, key, true);
    }

    void ReactiveEffect::AttachToWindow(Gorgon::Window &window) {
        DetachFromWindow();
        std::weak_ptr<Impl> wimpl = impl;
        impl->attached_window = &window;
        impl->window_key_token = window.KeyPreviewEvent.Register([wimpl](Input::Key key, float state) {
            if(state > 0.f) {
                if(auto l = wimpl.lock()) {
                    if(l->visible && l->owned_ctrl && l->owned_ctrl->IsPlaying() &&
                       (l->context == nullptr || l->context == &Gorgon::Animation::Governor::Active())) {
                        triggerReactiveKey(*l, key, false);
                    }
                }
            }
        });
    }

    void ReactiveEffect::DetachFromWindow() {
        if(impl->attached_window && impl->window_key_token) {
            impl->attached_window->KeyPreviewEvent.Unregister(impl->window_key_token);
            impl->attached_window  = nullptr;
            impl->window_key_token = 0;
        }
    }

    void ReactiveEffect::SetColor(Graphics::RGBA color) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->stops = {color};
        impl->random_reactive = false;
    }

    void ReactiveEffect::SetRandomColor(bool random) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->random_reactive = random;
    }

    void ReactiveEffect::SetFadeDuration(unsigned fade_ms) {
        SetDuration(fade_ms);
    }

    void ReactiveEffect::SetRippleRadius(float radius_cells) {
        std::lock_guard<std::mutex> g(S().state_mtx);
        impl->ripple_radius = std::max(0.f, radius_cells);
    }

    // -------------------------------------------------------------
    // Context (Governor / Scene Scoped OpenRGB Interface)
    // -------------------------------------------------------------

    std::vector<Device> Context::GetDevices(bool only_connected) const {
        std::vector<Device> list;
        if(!ensure())
            return list;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(!only_connected || d->IsConnected())
                list.push_back(Access::makeDevice(Access::impl(*d), governor));
        }

        return list;
    }

    std::vector<KeyboardDevice> Context::GetKeyboards(bool only_connected) const {
        std::vector<KeyboardDevice> list;
        if(!ensure())
            return list;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(auto kb = dynamic_cast<KeyboardDevice *>(d.get())) {
                if(!only_connected || kb->IsConnected())
                    list.push_back(Access::makeKeyboard(Access::impl(*kb), governor));
            }
        }

        return list;
    }

    std::vector<Device> Context::GetStrips(bool only_connected) const {
        std::vector<Device> list;
        if(!ensure())
            return list;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(d->GetType() == DeviceType::LedStrip) {
                if(!only_connected || d->IsConnected())
                    list.push_back(Access::makeDevice(Access::impl(*d), governor));
            }
        }

        return list;
    }

    std::vector<Zone> Context::GetStripZones(bool only_connected) const {
        std::vector<Zone> list;
        if(!ensure())
            return list;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(!only_connected || d->IsConnected()) {
                for(const auto &z : Access::impl(*d)->zones) {
                    if(z.GetType() == ZoneType::Linear)
                        list.push_back(Access::makeZone(z, governor));
                }
            }
        }
        return list;
    }

    std::vector<Zone> Context::GetSingleLEDZones(bool only_connected) const {
        std::vector<Zone> list;
        if(!ensure())
            return list;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(!only_connected || d->IsConnected()) {
                for(const auto &z : Access::impl(*d)->zones) {
                    if(z.GetType() == ZoneType::Single)
                        list.push_back(Access::makeZone(z, governor));
                }
            }
        }
        return list;
    }

    std::vector<Zone> Context::GetAllZones(bool only_connected) const {
        std::vector<Zone> list;
        if(!ensure())
            return list;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(!only_connected || d->IsConnected()) {
                for(const auto &z : Access::impl(*d)->zones) {
                    list.push_back(Access::makeZone(z, governor));
                }
            }
        }
        return list;
    }

    std::optional<Device> Context::FindDevice(const std::string &name) const {
        if(!ensure())
            return std::nullopt;

        auto n = String::ToLower(name);

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(d->IsConnected() && String::ToLower(d->GetName()) == n)
                return Access::makeDevice(Access::impl(*d), governor);
        }

        for(auto &d : S().devices) {
            if(d->IsConnected() && String::ToLower(d->GetName()).find(n) != std::string::npos)
                return Access::makeDevice(Access::impl(*d), governor);
        }

        for(auto &d : S().devices) {
            if(String::ToLower(d->GetName()) == n)
                return Access::makeDevice(Access::impl(*d), governor);
        }

        for(auto &d : S().devices) {
            if(String::ToLower(d->GetName()).find(n) != std::string::npos)
                return Access::makeDevice(Access::impl(*d), governor);
        }

        return std::nullopt;
    }

    std::optional<KeyboardDevice> Context::GetPrimaryKeyboard() const {
        if(!ensure())
            return std::nullopt;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(d->IsConnected()) {
                if(auto kb = dynamic_cast<KeyboardDevice *>(d.get()))
                    return Access::makeKeyboard(Access::impl(*kb), governor);
            }
        }

        for(auto &d : S().devices) {
            if(auto kb = dynamic_cast<KeyboardDevice *>(d.get()))
                return Access::makeKeyboard(Access::impl(*kb), governor);
        }

        return std::nullopt;
    }

    std::optional<Device> Context::GetPrimaryStrip() const {
        if(!ensure())
            return std::nullopt;

        std::lock_guard<std::mutex> g(S().state_mtx);
        for(auto &d : S().devices) {
            if(d->IsConnected() && d->GetType() == DeviceType::LedStrip)
                return Access::makeDevice(Access::impl(*d), governor);
        }

        for(auto &d : S().devices) {
            if(d->GetType() == DeviceType::LedStrip)
                return Access::makeDevice(Access::impl(*d), governor);
        }

        return std::nullopt;
    }

    void Context::SetKeyColor(Input::Key key, Graphics::RGBA color) const {
        for(auto &kb : GetKeyboards())
            kb.SetKeyColor(key, color);
    }

    void Context::SetKeyColor(const KeyGroup &group, Graphics::RGBA color) const {
        for(auto &kb : GetKeyboards())
            kb.SetKeyGroupColor(group, color);
    }

    bool Context::HasKey(Input::Key key) const {
        for(auto &kb : GetKeyboards()) {
            if(kb.HasKey(key))
                return true;
        }
        return false;
    }

    bool Context::HasKey(const std::string &key_name) const {
        for(auto &kb : GetKeyboards()) {
            if(kb.HasKey(key_name))
                return true;
        }
        return false;
    }

    std::vector<Input::Key> Context::GetKeys() const {
        if(auto kb = GetPrimaryKeyboard())
            return kb->GetKeys();
        return {};
    }

    void Context::Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset, BlendMode blend) const {
        for(auto &d : GetDevices()) {
            for(auto &z : d.GetZones()) {
                if(z.IsMatrix())
                    z.Draw(bitmap, offset, blend);
            }
        }
    }

    void Context::Draw(const Containers::Image &image, Geometry::Point offset, BlendMode blend) const {
        for(auto &d : GetDevices()) {
            for(auto &z : d.GetZones()) {
                if(z.IsMatrix())
                    z.Draw(image, offset, blend);
            }
        }
    }

    void Context::DrawScaled(const Graphics::Bitmap &bitmap) const {
        DrawScaled(bitmap, Containers::InterpolationMethod::Linear, BlendMode::Overwrite);
    }

    void Context::DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp, BlendMode blend) const {
        for(auto &d : GetDevices()) {
            for(auto &z : d.GetZones()) {
                if(z.IsMatrix())
                    z.DrawScaled(bitmap, interp, blend);
            }
        }
    }

    void Context::DrawScaled(const Containers::Image &image) const {
        DrawScaled(image, Containers::InterpolationMethod::Linear, BlendMode::Overwrite);
    }

    void Context::DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp, BlendMode blend) const {
        for(auto &d : GetDevices()) {
            for(auto &z : d.GetZones()) {
                if(z.IsMatrix())
                    z.DrawScaled(image, interp, blend);
            }
        }
    }

    void Context::SetAllColor(Graphics::RGBA color) const {
        for(auto &d : GetDevices())
            d.SetColor(color);
    }

    void Context::ClearAll() const {
        for(auto &d : GetDevices())
            d.Clear();
    }

    void Context::RestoreAll() const {
        if(!S().initialized)
            return;

        if(!governor) {
            OpenRGB::RestoreAll();
            return;
        }

        std::lock_guard<std::mutex> sg(S().state_mtx);
        for(auto &d : deviceList()) {
            if(d->gov_layers.erase(governor) > 0) {
                d->dirty = true;
                d->revision++;
            }
        }
        S().wake.notify_all();
    }

    void Context::Suspend() const {
        OpenRGB::Suspend();
    }

    void Context::Resume() const {
        OpenRGB::Resume();
    }

    bool Context::IsSuspended() const {
        return OpenRGB::IsSuspended();
    }

    StateSnapshot Context::CaptureState() const {
        return OpenRGB::CaptureState();
    }

    void Context::RestoreState(const StateSnapshot &state) const {
        OpenRGB::RestoreState(state);
    }

    Progressbar Context::CreateProgressbar() const {
        Progressbar pb;
        AttachProgressbar(pb);
        return pb;
    }

    Progressbar Context::CreateProgressbar(const KeyGroup &keys, Graphics::RGBA color, Graphics::RGBA bg) const {
        Progressbar pb(keys, color, bg);
        AttachProgressbar(pb);
        return pb;
    }

    Progressbar Context::CreateProgressbar(const KeyGroup &keys, const KeyboardDevice &keyboard, Graphics::RGBA color, Graphics::RGBA bg) const {
        Progressbar pb(keys, keyboard, color, bg);
        AttachProgressbar(pb);
        return pb;
    }

    Progressbar Context::CreateProgressbar(const KeyGroup &keys, const Zone &zone, Graphics::RGBA color, Graphics::RGBA bg) const {
        Progressbar pb(keys, zone, color, bg);
        AttachProgressbar(pb);
        return pb;
    }

    Progressbar Context::CreateProgressbar(const Zone &zone, Graphics::RGBA color, Graphics::RGBA bg) const {
        Progressbar pb(zone, color, bg);
        AttachProgressbar(pb);
        return pb;
    }

    Progressbar Context::CreateProgressbar(const Device &device, Graphics::RGBA color, Graphics::RGBA bg) const {
        Progressbar pb(device, color, bg);
        AttachProgressbar(pb);
        return pb;
    }

    void Context::AttachProgressbar(Progressbar &progressbar) const {
        std::lock_guard<std::mutex> g(S().state_mtx);
        auto p = Access::impl(progressbar);
        if(p && p->context != governor) {
            p->context = governor;
            markAllDevicesDirtyLocked();
        }
    }

    void Context::AttachAnimation(Animation &animation) const {
        auto a = Access::impl(animation);
        if(!a) return;

        if(governor)
            a->owned_ctrl->SetGovernor(*const_cast<Gorgon::Animation::Governor *>(governor));
        else
            a->owned_ctrl->SetGovernor(Gorgon::Animation::Governor::Default());

        std::lock_guard<std::mutex> g(S().state_mtx);
        if(a->context != governor) {
            a->context = governor;
            markAllDevicesDirtyLocked();
        }
    }

    namespace internal {
        std::shared_ptr<ShortcutOverlay> registerShortcutOverlay(const Gorgon::Animation::Governor *context) {
            auto overlay = std::make_shared<ShortcutOverlay>();
            overlay->context = context;
            overlay->active = true;
            overlay->mode = ModifierHighlightMode::ActiveModifiers;
            {
                std::lock_guard<std::mutex> g(S().state_mtx);
                S().shortcut_overlays.push_back(overlay);
                markAllDevicesDirtyLocked();
            }
            return overlay;
        }

        void updateShortcutOverlay(const std::shared_ptr<ShortcutOverlay> &overlay) {
            if(!overlay) return;
            ensure();
            std::lock_guard<std::mutex> g(S().state_mtx);
            markAllDevicesDirtyLocked();
        }

        void unregisterShortcutOverlay(const std::shared_ptr<ShortcutOverlay> &overlay) {
            if(!overlay) return;
            std::lock_guard<std::mutex> g(S().state_mtx);
            overlay->active = false;
            auto &vec = S().shortcut_overlays;
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                               [&](const std::weak_ptr<ShortcutOverlay> &w) {
                                   auto sp = w.lock();
                                   return !sp || sp == overlay;
                               }),
                vec.end());
            markAllDevicesDirtyLocked();
        }
    }

    void Context::SetActionKeys(const std::vector<internal::ShortcutBinding> &bindings, ModifierHighlightMode mode) const {
        auto &s = S();
        std::shared_ptr<internal::ShortcutOverlay> overlay;
        {
            std::lock_guard<std::mutex> g(s.state_mtx);
            auto it = s.context_action_keys.find(governor);
            if(it != s.context_action_keys.end()) {
                overlay = it->second;
            }
        }
        if(!overlay) {
            overlay = internal::registerShortcutOverlay(governor);
            std::lock_guard<std::mutex> g(s.state_mtx);
            s.context_action_keys[governor] = overlay;
        }
        overlay->mode = mode;
        overlay->bindings = bindings;
        overlay->active = true;
        internal::updateShortcutOverlay(overlay);
    }

    void Context::ClearActionKeys() const {
        auto &s = S();
        std::shared_ptr<internal::ShortcutOverlay> overlay;
        {
            std::lock_guard<std::mutex> g(s.state_mtx);
            auto it = s.context_action_keys.find(governor);
            if(it != s.context_action_keys.end()) {
                overlay = it->second;
                s.context_action_keys.erase(it);
            }
        }
        if(overlay) {
            internal::unregisterShortcutOverlay(overlay);
        }
    }

    // -------------------------------------------------------------
    // Global functions (Always Modify Global Layer)
    // -------------------------------------------------------------

    bool Initialize(const std::string &host, uint16_t port, bool suspended) {
        if(suspended)
            S().suspended = true;
        return connectNow(host, port, true);
    }

    bool Initialize(bool suspended) {
        return Initialize("127.0.0.1", 6742, suspended);
    }

    bool IsAvailable() {
        return ensure() && S().connected;
    }

    void Shutdown(bool restore) {
        auto &s = S();
        std::lock_guard<std::mutex> ig(s.init_mtx);

        if(!s.initialized)
            return;

        if(!restore)
            tick();

        stopWorker();

        {
            std::lock_guard<std::mutex> cg(s.client_mtx);
            if(restore)
                restoreLocked();
            s.client.Disconnect();
            s.connected = false;
        }

        {
            std::lock_guard<std::mutex> sg(s.state_mtx);
            s.devices.clear();
            s.progressbars.clear();
            s.animations.clear();
            s.shortcut_overlays.clear();
            s.context_action_keys.clear();
        }

        s.initialized = false;
        Logger << "OpenRGB shut down" << (restore ? " (restored)" : " (kept state)");
    }

    void RestoreAll() {
        if(!S().initialized)
            return;

        std::lock_guard<std::mutex> cg(S().client_mtx);
        restoreLocked();
    }

    void Suspend() {
        auto &s = S();
        if(s.suspended.exchange(true))
            return;

        if(!s.initialized) {
            Logger << "OpenRGB suspended (pre-initialization)";
            return;
        }

        std::lock_guard<std::mutex> cg(s.client_mtx);
        if(!s.client.IsConnected())
            return;

        std::vector<std::shared_ptr<DImpl>> list;
        {
            std::lock_guard<std::mutex> sg(s.state_mtx);
            for(auto &d : deviceList()) {
                if(d->hw_touched && d->connected && d->id >= 0)
                    list.push_back(d);
            }
        }

        for(auto &d : list) {
            restoreDeviceHardwareLocked(d);

            std::lock_guard<std::mutex> sg(s.state_mtx);
            d->engaged    = false;
            d->hw_touched = false;
            d->hw_mode    = d->orig_mode;
            d->last_sent.clear();
        }

        Logger << "OpenRGB suspended: hardware restored to OEM profile";
    }

    void Resume() {
        auto &s = S();
        if(!s.suspended.exchange(false))
            return;

        if(!s.initialized) {
            ensure();
        }

        {
            std::lock_guard<std::mutex> sg(s.state_mtx);
            for(auto &d : deviceList()) {
                if(d->connected && d->id >= 0) {
                    d->dirty = true;
                    d->revision++;
                }
            }
        }

        s.wake.notify_all();
        Logger << "OpenRGB resumed: software RGB lighting restored";
    }

    bool IsSuspended() {
        return S().suspended.load();
    }

    StateSnapshot CaptureState() {
        StateSnapshot snap;
        auto &s = S();
        if(!s.initialized)
            return snap;

        std::lock_guard<std::mutex> sg(s.state_mtx);
        for(const auto &d : deviceList()) {
            if(!d->connected || d->id < 0) continue;
            StateSnapshot::DeviceState ds;
            ds.id = d->id;
            ds.name = d->name;
            ds.active_mode = d->global_layer.active_mode;
            if(ds.active_mode >= 0 && ds.active_mode < (int)d->global_layer.modes.size()) {
                const auto &m = d->global_layer.modes[ds.active_mode];
                ds.speed = m.speed;
                ds.brightness = m.brightness;
                ds.direction = m.direction;
                ds.color_mode = m.color_mode;
                ds.mode_colors.reserve(m.colors.size());
                for(const auto &c : m.colors)
                    ds.mode_colors.emplace_back(c.R(), c.G(), c.B(), Byte(255));
            }
            ds.colors = d->global_layer.base_leds;
            snap.devices.push_back(std::move(ds));
        }
        return snap;
    }

    void RestoreState(const StateSnapshot &state) {
        auto &s = S();
        if(!s.initialized)
            return;

        {
            std::lock_guard<std::mutex> sg(s.state_mtx);
            for(const auto &ds : state.devices) {
                for(auto &d : deviceList()) {
                    if(d->id == ds.id || (ds.id < 0 && d->name == ds.name)) {
                        d->global_layer.active_mode = ds.active_mode;
                        if(ds.active_mode >= 0 && ds.active_mode < (int)d->global_layer.modes.size()) {
                            auto &m = d->global_layer.modes[ds.active_mode];
                            m.speed = ds.speed;
                            m.brightness = ds.brightness;
                            m.direction = ds.direction;
                            m.color_mode = ds.color_mode;
                            if(!ds.mode_colors.empty()) {
                                m.colors.clear();
                                for(const auto &c : ds.mode_colors)
                                    m.colors.push_back(wire(c));
                            }
                        }
                        d->global_layer.base_leds = ds.colors;
                        d->global_layer.touched = true;
                        d->global_layer.mode_dirty = true;
                        d->dirty = true;
                        d->revision++;
                        break;
                    }
                }
            }
        }

        if(!s.suspended.load())
            s.wake.notify_all();
    }

    void Flush() {
        if(S().suspended.load())
            return;

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
        return GlobalContext().GetDevices(only_connected);
    }

    std::vector<KeyboardDevice> GetKeyboards(bool only_connected) {
        return GlobalContext().GetKeyboards(only_connected);
    }

    std::vector<Device> GetStrips(bool only_connected) {
        return GlobalContext().GetStrips(only_connected);
    }

    std::vector<Zone> GetStripZones(bool only_connected) {
        return GlobalContext().GetStripZones(only_connected);
    }

    std::vector<Zone> GetSingleLEDZones(bool only_connected) {
        return GlobalContext().GetSingleLEDZones(only_connected);
    }

    std::vector<Zone> GetAllZones(bool only_connected) {
        return GlobalContext().GetAllZones(only_connected);
    }

    std::optional<Device> FindDevice(const std::string &name) {
        return GlobalContext().FindDevice(name);
    }

    std::optional<KeyboardDevice> GetPrimaryKeyboard() {
        return GlobalContext().GetPrimaryKeyboard();
    }

    std::optional<Device> GetPrimaryStrip() {
        return GlobalContext().GetPrimaryStrip();
    }

    void SetKeyColor(Input::Key key, Graphics::RGBA color) {
        GlobalContext().SetKeyColor(key, color);
    }

    void SetKeyGroupColor(const KeyGroup &group, Graphics::RGBA color) {
        GlobalContext().SetKeyColor(group, color);
    }

    bool HasKey(Input::Key key) {
        return GlobalContext().HasKey(key);
    }

    bool HasKey(const std::string &key_name) {
        return GlobalContext().HasKey(key_name);
    }

    std::vector<Input::Key> GetKeys() {
        return GlobalContext().GetKeys();
    }

    void Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset, BlendMode blend) {
        GlobalContext().Draw(bitmap, offset, blend);
    }

    void Draw(const Containers::Image &image, Geometry::Point offset, BlendMode blend) {
        GlobalContext().Draw(image, offset, blend);
    }

    void DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp, BlendMode blend) {
        GlobalContext().DrawScaled(bitmap, interp, blend);
    }

    void DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp, BlendMode blend) {
        GlobalContext().DrawScaled(image, interp, blend);
    }

    void SetAllColor(Graphics::RGBA color) {
        GlobalContext().SetAllColor(color);
    }

    void ClearAll() {
        GlobalContext().ClearAll();
    }

    Progressbar CreateProgressbar() {
        return GlobalContext().CreateProgressbar();
    }

    Progressbar CreateProgressbar(const KeyGroup &keys, Graphics::RGBA color, Graphics::RGBA bg) {
        return GlobalContext().CreateProgressbar(keys, color, bg);
    }

    Progressbar CreateProgressbar(const KeyGroup &keys, const KeyboardDevice &keyboard, Graphics::RGBA color, Graphics::RGBA bg) {
        return GlobalContext().CreateProgressbar(keys, keyboard, color, bg);
    }

    Progressbar CreateProgressbar(const KeyGroup &keys, const Zone &zone, Graphics::RGBA color, Graphics::RGBA bg) {
        return GlobalContext().CreateProgressbar(keys, zone, color, bg);
    }

    Progressbar CreateProgressbar(const Zone &zone, Graphics::RGBA color, Graphics::RGBA bg) {
        return GlobalContext().CreateProgressbar(zone, color, bg);
    }

    Progressbar CreateProgressbar(const Device &device, Graphics::RGBA color, Graphics::RGBA bg) {
        return GlobalContext().CreateProgressbar(device, color, bg);
    }

    void AttachProgressbar(Progressbar &progressbar) {
        GlobalContext().AttachProgressbar(progressbar);
    }

    void AttachAnimation(Animation &animation) {
        GlobalContext().AttachAnimation(animation);
    }

    void SetActionKeys(const std::vector<internal::ShortcutBinding> &bindings, ModifierHighlightMode mode) {
        GlobalContext().SetActionKeys(bindings, mode);
    }

    void ClearActionKeys() {
        GlobalContext().ClearActionKeys();
    }

} // namespace Gorgon::Devices::OpenRGB
