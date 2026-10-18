/**
 * @file openrgb_raw.cpp
 * @brief Manual test application for OpenRGB raw protocol client.
 *
 * Test Sequence:
 * 1. Enumerate all controllers.
 * 2. Select target keyboard and backup initial mode and LED colors.
 * 3. Switch to Direct mode.
 * 4. Step 1: Set Escape key to RED using UpdateSingleLED (single-LED write). Wait for [Enter].
 * 5. Step 2: Highlight WASD (Green) + Space (Cyan) using UpdateLEDs (entire region write). Wait for [Enter].
 * 6. Step 3: Restore original colors and mode, then disconnect.
 */

#include <Gorgon/Devices/OpenRGB/OpenRGBClient.h>

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <thread>
#include <chrono>
#include <csignal>
#include <functional>

namespace {

    std::function<void()> g_restore_handler;

    void SignalHandler(int signum) {
        if (g_restore_handler) {
            std::cout << "\n\n[!] Interrupted by Signal (" << signum << "). Restoring original hardware state before exiting..." << std::endl;
            g_restore_handler();
        }
        std::_Exit(128 + signum);
    }

    std::string ToLower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return str;
    }

    const char *DeviceTypeToString(openrgb::DeviceType type) {
        switch (type) {
            case openrgb::DeviceType::Motherboard:  return "Motherboard";
            case openrgb::DeviceType::DRAM:         return "DRAM";
            case openrgb::DeviceType::GPU:          return "GPU";
            case openrgb::DeviceType::Cooler:       return "Cooler";
            case openrgb::DeviceType::LedStrip:     return "LED Strip";
            case openrgb::DeviceType::Keyboard:     return "Keyboard";
            case openrgb::DeviceType::Mouse:        return "Mouse";
            case openrgb::DeviceType::MouseMat:     return "Mouse Mat";
            case openrgb::DeviceType::Headset:      return "Headset";
            case openrgb::DeviceType::HeadsetStand: return "Headset Stand";
            case openrgb::DeviceType::Gamepad:      return "Gamepad";
            case openrgb::DeviceType::Light:        return "Light";
            case openrgb::DeviceType::Speaker:      return "Speaker";
            case openrgb::DeviceType::Virtual:      return "Virtual";
            case openrgb::DeviceType::Unknown:      return "Unknown";
            default:                                return "Other";
        }
    }

    bool IsEscapeKeyName(const std::string &name) {
        std::string lower = ToLower(name);
        return lower == "key: escape" || lower == "key: esc" ||
               lower.find("escape") != std::string::npos ||
               lower == "esc";
    }

    std::vector<int> FindKeyLEDIndices(const openrgb::Controller &ctrl, const std::string &key_name) {
        std::vector<int> indices;
        std::string target = ToLower(key_name);
        for (size_t i = 0; i < ctrl.leds.size(); ++i) {
            std::string led_lower = ToLower(ctrl.leds[i].name);
            if (led_lower == target || led_lower == "key: " + target) {
                indices.push_back(static_cast<int>(i));
            } else if (target == "space" && (led_lower == "key: space" || led_lower == "key: spacebar" || led_lower == "space" || led_lower == "spacebar")) {
                indices.push_back(static_cast<int>(i));
            }
        }
        return indices;
    }

    bool LooksLikeKeyboard(const openrgb::Controller &ctrl) {
        if (ctrl.type == openrgb::DeviceType::Keyboard) return true;
        std::string lower_name = ToLower(ctrl.name);
        if (lower_name.find("keyboard") != std::string::npos || lower_name.find("kbd") != std::string::npos) {
            return true;
        }
        for (const auto &led : ctrl.leds) {
            std::string l_lower = ToLower(led.name);
            if (l_lower.rfind("key:", 0) == 0) {
                return true;
            }
        }
        return false;
    }

} // anonymous namespace

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << " Gorgon Engine - OpenRGB Raw Client Test" << std::endl;
    std::cout << "========================================" << std::endl;

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    openrgb::Client client;

    std::cout << "\n[1/7] Connecting to OpenRGB server (127.0.0.1:6742)..." << std::endl;
    if (!client.Connect("127.0.0.1", 6742, 3000)) {
        std::cerr << "[-] Error connecting to OpenRGB server: " << client.GetLastError() << std::endl;
        std::cerr << "    Make sure the OpenRGB application is running and the SDK server is started." << std::endl;
        return 1;
    }

    uint32_t protocol = client.RequestProtocolVersion(5);
    std::cout << "[+] Connected! Negotiated Protocol Version: " << protocol << std::endl;

    if (!client.SetClientName("Gorgon OpenRGB Raw Test")) {
        std::cerr << "[-] Failed to set client name: " << client.GetLastError() << std::endl;
    }

    if (!client.SetClientFlags(openrgb::ClientFlags::SupportsRGBController |
                              openrgb::ClientFlags::SupportsProfileManager |
                              openrgb::ClientFlags::RequestLocalClient)) {
        std::cerr << "[-] Failed to set client flags: " << client.GetLastError() << std::endl;
    }

    std::cout << "\n[2/7] Enumerating connected RGB controllers..." << std::endl;
    int32_t count = client.RequestControllerCount();
    if (count < 0) {
        std::cerr << "[-] Failed to retrieve controller count: " << client.GetLastError() << std::endl;
        client.Disconnect();
        return 1;
    }

    std::cout << "[+] Found " << count << " controller(s) on server:" << std::endl;

    int keyboard_id = -1;
    openrgb::Controller kb_ctrl;
    int esc_led_idx = -1;

    for (int32_t i = 0; i < count; ++i) {
        openrgb::Controller ctrl;
        if (!client.RequestControllerData(static_cast<uint32_t>(i), ctrl)) {
            std::cerr << "    [" << i << "] Failed to retrieve data: " << client.GetLastError() << std::endl;
            continue;
        }

        std::string active_mode_name = "N/A";
        if (ctrl.active_mode >= 0 && ctrl.active_mode < static_cast<int>(ctrl.modes.size())) {
            active_mode_name = ctrl.modes[ctrl.active_mode].name;
        }

        std::cout << "\n    [" << i << "] " << ctrl.name << std::endl;
        std::cout << "        Vendor:      " << (ctrl.vendor.empty() ? "(none)" : ctrl.vendor) << std::endl;
        std::cout << "        Type:        " << DeviceTypeToString(ctrl.type) << std::endl;
        std::cout << "        Description: " << (ctrl.description.empty() ? "(none)" : ctrl.description) << std::endl;
        std::cout << "        Version:     " << (ctrl.version.empty() ? "(none)" : ctrl.version) << std::endl;
        std::cout << "        LEDs:        " << ctrl.leds.size() << std::endl;
        std::cout << "        Zones:       " << ctrl.zones.size() << std::endl;
        std::cout << "        Modes:       " << ctrl.modes.size() << " (Active: \"" << active_mode_name << "\")" << std::endl;

        // Check if this controller can be our test keyboard
        if (keyboard_id == -1 && LooksLikeKeyboard(ctrl)) {
            for (size_t l = 0; l < ctrl.leds.size(); ++l) {
                if (IsEscapeKeyName(ctrl.leds[l].name)) {
                    keyboard_id = i;
                    kb_ctrl = ctrl;
                    esc_led_idx = static_cast<int>(l);
                    break;
                }
            }
        }
    }

    if (keyboard_id == -1 || esc_led_idx == -1) {
        std::cout << "\n[-] No keyboard device with an Escape key LED was identified among the devices." << std::endl;
        std::cout << "    Exiting test." << std::endl;
        client.Disconnect();
        return 0;
    }

    std::cout << "\n[3/7] Target Keyboard Selected:" << std::endl;
    std::cout << "    Controller: [" << keyboard_id << "] \"" << kb_ctrl.name << "\"" << std::endl;
    std::cout << "    Escape LED: [" << esc_led_idx << "] \"" << kb_ctrl.leds[esc_led_idx].name << "\"" << std::endl;

    // -------------------------------------------------------------
    // Snapshot original hardware state in memory
    // -------------------------------------------------------------
    int32_t original_mode_idx = kb_ctrl.active_mode;
    std::string original_mode_name = (original_mode_idx >= 0 && original_mode_idx < static_cast<int>(kb_ctrl.modes.size()))
                                     ? kb_ctrl.modes[original_mode_idx].name : "N/A";
    openrgb::Mode original_mode = (original_mode_idx >= 0 && original_mode_idx < static_cast<int>(kb_ctrl.modes.size()))
                                  ? kb_ctrl.modes[original_mode_idx] : openrgb::Mode{};
    std::vector<openrgb::Color> original_colors = kb_ctrl.colors;

    bool was_direct = (original_mode_name == "Direct") ||
                      ((original_mode.flags & openrgb::ModeFlags::HasPerLedColor) &&
                       original_mode.color_mode == static_cast<uint32_t>(openrgb::ModeColorMode::PerLed));

    std::cout << "    Original Mode:     [" << original_mode_idx << "] \"" << original_mode_name << "\"" << std::endl;
    std::cout << "    Mode Type:         " << (was_direct ? "Direct / Per-LED" : "Hardware Effect (Mode-specific)") << std::endl;
    if (was_direct) {
        std::cout << "    Direct Colors:     " << original_colors.size() << " LEDs captured" << std::endl;
    } else {
        std::cout << "    Mode Settings:     Speed=" << original_mode.speed << ", Brightness=" << original_mode.brightness
                  << ", Direction=" << original_mode.direction << ", Colors=" << original_mode.colors.size() << std::endl;
        if (!original_mode.colors.empty()) {
            std::cout << "    Mode Base Color 0: RGB(" << static_cast<int>(original_mode.colors[0].R()) << ", "
                      << static_cast<int>(original_mode.colors[0].G()) << ", "
                      << static_cast<int>(original_mode.colors[0].B()) << ")" << std::endl;
        }
    }

    std::vector<openrgb::Color> black_colors(kb_ctrl.leds.size(), openrgb::Color::FromRGB(0, 0, 0));

    auto RestoreHardware = [&]() {
        if (!client.IsConnected()) return;
        std::cout << "\n[Restoration] Restoring original hardware state..." << std::endl;

        if (was_direct) {
            std::cout << "    Restoring original Direct LED colors (" << original_colors.size() << " LEDs)..." << std::endl;
            client.UpdateLEDs(static_cast<uint32_t>(keyboard_id), original_colors);
            if (original_mode_idx >= 0 && original_mode_idx < static_cast<int>(kb_ctrl.modes.size())) {
                client.UpdateMode(static_cast<uint32_t>(keyboard_id), original_mode_idx, original_mode);
            }
        } else {
            // Hardware effect mode (e.g. Reactive, Breathing, Rainbow Wave)
            // 1. Wipe direct LED registers on physical hardware to black so they don't override the effect
            std::cout << "    Clearing Direct mode LEDs on hardware..." << std::endl;
            client.UpdateLEDs(static_cast<uint32_t>(keyboard_id), black_colors);

            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            // 2. Re-apply original hardware mode and all mode parameters
            if (original_mode_idx >= 0 && original_mode_idx < static_cast<int>(kb_ctrl.modes.size())) {
                std::cout << "    Restoring original hardware mode [" << original_mode_idx << "] \"" << original_mode_name << "\"..." << std::endl;
                bool ok = client.UpdateMode(static_cast<uint32_t>(keyboard_id), original_mode_idx, original_mode);
                if (ok) {
                    std::cout << "    [+] Original mode and parameters reinstated successfully." << std::endl;
                } else {
                    std::cerr << "    [-] UpdateMode restore failed: " << client.GetLastError() << std::endl;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    };

    g_restore_handler = RestoreHardware;

    struct HardwareScopeGuard {
        std::function<void()> restore_fn;
        bool active = true;

        ~HardwareScopeGuard() {
            if (active && restore_fn) {
                std::cout << "\n[RAII] Scope exit: ensuring hardware state is restored..." << std::endl;
                restore_fn();
            }
        }
    } scope_guard{ RestoreHardware };

    // Switch to Direct mode if available
    int direct_mode_idx = -1;
    for (size_t m = 0; m < kb_ctrl.modes.size(); ++m) {
        if (kb_ctrl.modes[m].name == "Direct") {
            direct_mode_idx = static_cast<int>(m);
            break;
        }
    }

    if (direct_mode_idx >= 0) {
        std::cout << "    Switching to 'Direct' mode (mode index " << direct_mode_idx << ")..." << std::endl;
        if (!client.UpdateMode(static_cast<uint32_t>(keyboard_id), direct_mode_idx, kb_ctrl.modes[direct_mode_idx])) {
            std::cerr << "[-] UpdateMode(Direct) returned failure: " << client.GetLastError() << std::endl;
        } else {
            std::cout << "[+] Switched to Direct mode." << std::endl;
        }
    } else {
        std::cout << "    [!] Warning: 'Direct' mode not found in modes list, attempting to proceed in current mode." << std::endl;
    }

    // Clear all keys to unlit/black so previous leftover states are wiped clean
    client.UpdateLEDs(static_cast<uint32_t>(keyboard_id), black_colors);

    // -------------------------------------------------------------
    // Test Step 1: Single LED write (Escape in Red)
    // -------------------------------------------------------------
    std::cout << "\n[4/7] TEST 1: Setting Escape key LED to RED via UpdateSingleLED..." << std::endl;
    openrgb::Color red = openrgb::Color::FromRGB(255, 0, 0);

    bool single_ok = client.UpdateSingleLED(static_cast<uint32_t>(keyboard_id), esc_led_idx, red);
    if (single_ok) {
        std::cout << "[+] Escape key LED updated to RED." << std::endl;
    } else {
        std::cerr << "[-] UpdateSingleLED failed: " << client.GetLastError() << std::endl;
    }

    std::cout << "\n>>> [CHECK 1] Is Escape RED? Press [Enter] to proceed to WASD + Space test... <<<" << std::flush;
    std::cin.get();

    // -------------------------------------------------------------
    // Test Step 2: Entire region / device write (WASD in Green, Space in Cyan)
    // -------------------------------------------------------------
    std::cout << "\n[5/7] TEST 2: Highlighting WASD + Space via full device write (UpdateLEDs)..." << std::endl;

    std::vector<int> w_keys = FindKeyLEDIndices(kb_ctrl, "w");
    std::vector<int> a_keys = FindKeyLEDIndices(kb_ctrl, "a");
    std::vector<int> s_keys = FindKeyLEDIndices(kb_ctrl, "s");
    std::vector<int> d_keys = FindKeyLEDIndices(kb_ctrl, "d");
    std::vector<int> space_keys = FindKeyLEDIndices(kb_ctrl, "space");

    std::cout << "    Found LED indices:" << std::endl;
    std::cout << "      W:     " << (w_keys.empty() ? -1 : w_keys[0]) << std::endl;
    std::cout << "      A:     " << (a_keys.empty() ? -1 : a_keys[0]) << std::endl;
    std::cout << "      S:     " << (s_keys.empty() ? -1 : s_keys[0]) << std::endl;
    std::cout << "      D:     " << (d_keys.empty() ? -1 : d_keys[0]) << std::endl;
    std::cout << "      Space: " << space_keys.size() << " LED(s)" << std::endl;

    // Create full region color buffer: Start with all LEDs off (black), set WASD=Green, Space=Cyan
    // This demonstrates both lighting up specific keys and turning off the previously lit Escape key.
    std::vector<openrgb::Color> region_colors = black_colors;
    openrgb::Color green = openrgb::Color::FromRGB(0, 255, 0);
    openrgb::Color cyan  = openrgb::Color::FromRGB(0, 255, 255);

    for (int idx : w_keys)     if (idx >= 0 && idx < static_cast<int>(region_colors.size())) region_colors[idx] = green;
    for (int idx : a_keys)     if (idx >= 0 && idx < static_cast<int>(region_colors.size())) region_colors[idx] = green;
    for (int idx : s_keys)     if (idx >= 0 && idx < static_cast<int>(region_colors.size())) region_colors[idx] = green;
    for (int idx : d_keys)     if (idx >= 0 && idx < static_cast<int>(region_colors.size())) region_colors[idx] = green;
    for (int idx : space_keys) if (idx >= 0 && idx < static_cast<int>(region_colors.size())) region_colors[idx] = cyan;

    // Test full device bulk update
    bool bulk_ok = client.UpdateLEDs(static_cast<uint32_t>(keyboard_id), region_colors);
    if (bulk_ok) {
        std::cout << "[+] Full device write succeeded (" << region_colors.size() << " LEDs updated)!" << std::endl;
    } else {
        std::cerr << "[-] UpdateLEDs failed: " << client.GetLastError() << std::endl;
    }

    std::cout << "\n>>> [CHECK 2] Are WASD (Green) and Space (Cyan) illuminated (and Escape off)? Press [Enter] to proceed to 30 FPS animation test... <<<" << std::flush;
    std::cin.get();

    // -------------------------------------------------------------
    // Test Step 3: 30 FPS Linear Sliding Animation on Number Keys
    // -------------------------------------------------------------
    std::cout << "\n[6/7] TEST 3: 30 FPS Linear Sliding Animation on Number Keys..." << std::endl;
    std::cout << "    This test runs an interpolated light pulse across the number keys" << std::endl;
    std::cout << "    (1, 2, 3, 4, 5, 6, 7, 8, 9, 0) targeting 30 FPS." << std::endl;
    std::cout << "    It benchmarks both UpdateLEDs (bulk) and UpdateSingleLED (per-key)" << std::endl;
    std::cout << "    and measures network send latency and frame stability." << std::endl;

    std::vector<std::string> num_names = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    std::vector<int> num_indices;
    for (const auto &k : num_names) {
        auto idxs = FindKeyLEDIndices(kb_ctrl, k);
        for (int idx : idxs) {
            std::string name_lower = ToLower(kb_ctrl.leds[idx].name);
            if (name_lower == "key: " + k || name_lower == k) {
                num_indices.push_back(idx);
                break;
            }
        }
    }

    if (num_indices.size() < 2) {
        std::cout << "    [-] Could not find sufficient number keys on this keyboard. Skipping animation test." << std::endl;
    } else {
        std::cout << "    Identified " << num_indices.size() << " number keys: ";
        for (size_t i = 0; i < num_indices.size(); ++i) {
            std::cout << num_names[i] << "(#" << num_indices[i] << ") ";
        }
        std::cout << std::endl;

        std::cout << "\n>>> Press [Enter] to run the 30 FPS animation test (~6 seconds total)... <<<" << std::flush;
        std::cin.get();

        struct BenchmarkStats {
            int total_frames = 0;
            int late_frames = 0;
            double total_send_ms = 0.0;
            double max_send_ms = 0.0;
            double min_send_ms = 1e9;
            double total_frame_ms = 0.0;
            double max_frame_ms = 0.0;
        };

        const int frames_per_test = 90; // 3.0 seconds @ 30 FPS
        const auto target_frame_us = std::chrono::microseconds(1000000 / 30); // 33,333 us (30 FPS)
        const double key_count = static_cast<double>(num_indices.size());
        const double speed_keys_per_sec = 6.0; // Complete pass in ~1.5s

        using Clock = std::chrono::steady_clock;

        // ---------------------------------------------------------
        // Part 3A: Bulk Updates (UpdateLEDs)
        // ---------------------------------------------------------
        std::cout << "\n    Running Part 3A: Bulk Updates (UpdateLEDs) at 30 FPS (Gold pulse)..." << std::endl;
        BenchmarkStats stats_bulk;
        double pos = 0.0;
        double vel = speed_keys_per_sec;
        std::vector<openrgb::Color> anim_colors = black_colors;

        auto next_frame = Clock::now();
        auto start_bulk = Clock::now();

        for (int frame = 0; frame < frames_per_test; ++frame) {
            auto frame_start = Clock::now();

            // 1. Advance position (ping-pong between 0 and key_count - 1)
            double dt = 1.0 / 30.0;
            pos += vel * dt;
            if (pos >= key_count - 1.0) {
                pos = key_count - 1.0;
                vel = -std::abs(vel);
            } else if (pos <= 0.0) {
                pos = 0.0;
                vel = std::abs(vel);
            }

            // 2. Interpolate colors across number keys (Gold/Amber: 255, 160, 0)
            for (size_t i = 0; i < num_indices.size(); ++i) {
                double dist = std::abs(static_cast<double>(i) - pos);
                double weight = (dist < 1.0) ? (1.0 - dist) : 0.0;
                uint8_t r = static_cast<uint8_t>(255.0 * weight);
                uint8_t g = static_cast<uint8_t>(160.0 * weight);
                uint8_t b = 0;
                anim_colors[num_indices[i]] = openrgb::Color::FromRGB(r, g, b);
            }

            // 3. Dispatch bulk update
            auto send_start = Clock::now();
            client.UpdateLEDs(static_cast<uint32_t>(keyboard_id), anim_colors);
            auto send_end = Clock::now();

            double send_ms = std::chrono::duration<double, std::milli>(send_end - send_start).count();
            stats_bulk.total_send_ms += send_ms;
            stats_bulk.max_send_ms = std::max(stats_bulk.max_send_ms, send_ms);
            stats_bulk.min_send_ms = std::min(stats_bulk.min_send_ms, send_ms);
            stats_bulk.total_frames++;

            // 4. Regulate frame rate to exactly 30 FPS
            next_frame += target_frame_us;
            auto now = Clock::now();
            if (now < next_frame) {
                std::this_thread::sleep_until(next_frame);
            } else {
                if (std::chrono::duration<double, std::milli>(now - next_frame).count() > 2.0) {
                    stats_bulk.late_frames++;
                }
                next_frame = now;
            }

            auto frame_end = Clock::now();
            double frame_ms = std::chrono::duration<double, std::milli>(frame_end - frame_start).count();
            stats_bulk.total_frame_ms += frame_ms;
            stats_bulk.max_frame_ms = std::max(stats_bulk.max_frame_ms, frame_ms);
        }
        auto end_bulk = Clock::now();
        double elapsed_bulk_s = std::chrono::duration<double>(end_bulk - start_bulk).count();

        // ---------------------------------------------------------
        // Part 3B: Per-Key Updates (UpdateSingleLED)
        // ---------------------------------------------------------
        std::cout << "    Running Part 3B: Per-Key Updates (UpdateSingleLED) at 30 FPS (Cyan pulse)..." << std::endl;
        BenchmarkStats stats_single;
        pos = 0.0;
        vel = speed_keys_per_sec;
        std::vector<openrgb::Color> current_num_colors(num_indices.size(), openrgb::Color::FromRGB(0, 0, 0));

        // Blank number keys before starting per-key test
        client.UpdateLEDs(static_cast<uint32_t>(keyboard_id), black_colors);

        next_frame = Clock::now();
        auto start_single = Clock::now();

        for (int frame = 0; frame < frames_per_test; ++frame) {
            auto frame_start = Clock::now();

            // 1. Advance position
            double dt = 1.0 / 30.0;
            pos += vel * dt;
            if (pos >= key_count - 1.0) {
                pos = key_count - 1.0;
                vel = -std::abs(vel);
            } else if (pos <= 0.0) {
                pos = 0.0;
                vel = std::abs(vel);
            }

            // 2. Compute target colors and only send changed keys (Cyan: 0, 220, 255)
            auto send_start = Clock::now();
            for (size_t i = 0; i < num_indices.size(); ++i) {
                double dist = std::abs(static_cast<double>(i) - pos);
                double weight = (dist < 1.0) ? (1.0 - dist) : 0.0;
                uint8_t r = 0;
                uint8_t g = static_cast<uint8_t>(220.0 * weight);
                uint8_t b = static_cast<uint8_t>(255.0 * weight);
                openrgb::Color new_c = openrgb::Color::FromRGB(r, g, b);

                if (new_c != current_num_colors[i]) {
                    client.UpdateSingleLED(static_cast<uint32_t>(keyboard_id), num_indices[i], new_c);
                    current_num_colors[i] = new_c;
                }
            }
            auto send_end = Clock::now();

            double send_ms = std::chrono::duration<double, std::milli>(send_end - send_start).count();
            stats_single.total_send_ms += send_ms;
            stats_single.max_send_ms = std::max(stats_single.max_send_ms, send_ms);
            stats_single.min_send_ms = std::min(stats_single.min_send_ms, send_ms);
            stats_single.total_frames++;

            // 3. Regulate frame rate
            next_frame += target_frame_us;
            auto now = Clock::now();
            if (now < next_frame) {
                std::this_thread::sleep_until(next_frame);
            } else {
                if (std::chrono::duration<double, std::milli>(now - next_frame).count() > 2.0) {
                    stats_single.late_frames++;
                }
                next_frame = now;
            }

            auto frame_end = Clock::now();
            double frame_ms = std::chrono::duration<double, std::milli>(frame_end - frame_start).count();
            stats_single.total_frame_ms += frame_ms;
            stats_single.max_frame_ms = std::max(stats_single.max_frame_ms, frame_ms);
        }
        auto end_single = Clock::now();
        double elapsed_single_s = std::chrono::duration<double>(end_single - start_single).count();

        // Turn off all keys after animation
        client.UpdateLEDs(static_cast<uint32_t>(keyboard_id), black_colors);

        // ---------------------------------------------------------
        // Print Benchmark Report
        // ---------------------------------------------------------
        double actual_fps_bulk   = static_cast<double>(stats_bulk.total_frames) / elapsed_bulk_s;
        double avg_send_bulk     = stats_bulk.total_send_ms / stats_bulk.total_frames;
        double actual_fps_single = static_cast<double>(stats_single.total_frames) / elapsed_single_s;
        double avg_send_single   = stats_single.total_send_ms / stats_single.total_frames;

        std::cout << "\n    ==================================================================" << std::endl;
        std::cout << "                 30 FPS Animation Benchmark Results                   " << std::endl;
        std::cout << "    ==================================================================" << std::endl;
        std::cout << "    Metric                   | Part 3A: Bulk (UpdateLEDs) | Part 3B: Per-Key  " << std::endl;
        std::cout << "    -------------------------+----------------------------+-------------------" << std::endl;
        std::cout << "    Total Frames             | " << std::setw(26) << stats_bulk.total_frames << " | " << std::setw(17) << stats_single.total_frames << std::endl;
        std::cout << "    Target FPS               | " << std::setw(22) << "30.0 FPS" << " | " << std::setw(13) << "30.0 FPS" << std::endl;
        std::cout << "    Actual FPS               | " << std::setw(22) << (std::to_string(actual_fps_bulk).substr(0, 5) + " FPS") << " | " << std::setw(13) << (std::to_string(actual_fps_single).substr(0, 5) + " FPS") << std::endl;
        std::cout << "    Avg Send Time            | " << std::setw(24) << (std::to_string(avg_send_bulk).substr(0, 5) + " ms") << " | " << std::setw(15) << (std::to_string(avg_send_single).substr(0, 5) + " ms") << std::endl;
        std::cout << "    Max Send Time            | " << std::setw(24) << (std::to_string(stats_bulk.max_send_ms).substr(0, 5) + " ms") << " | " << std::setw(15) << (std::to_string(stats_single.max_send_ms).substr(0, 5) + " ms") << std::endl;
        std::cout << "    Late Frames (>35ms)      | " << std::setw(26) << stats_bulk.late_frames << " | " << std::setw(17) << stats_single.late_frames << std::endl;
        std::cout << "    ==================================================================" << std::endl;

        if (stats_bulk.late_frames == 0 && stats_single.late_frames == 0) {
            std::cout << "    [+] SUCCESS: Server and keyboard kept up perfectly at 30 FPS!" << std::endl;
        } else {
            std::cout << "    [!] WARNING: Server or hardware could not maintain full 30 FPS cadence." << std::endl;
            if (stats_bulk.late_frames > 0) {
                std::cout << "        Bulk update (UpdateLEDs) had " << stats_bulk.late_frames << " late frame(s)." << std::endl;
            }
            if (stats_single.late_frames > 0) {
                std::cout << "        Per-key update (UpdateSingleLED) had " << stats_single.late_frames << " late frame(s)." << std::endl;
            }
        }
    }

    std::cout << "\n>>> [CHECK 3] Did the light smoothly slide across number keys 1-0? Press [Enter] to restore status and exit... <<<" << std::flush;
    std::cin.get();

    // -------------------------------------------------------------
    // Test Step 4: Restore original state
    // -------------------------------------------------------------
    std::cout << "\n[7/7] Restoring original status..." << std::endl;

    scope_guard.active = false; // Disarm RAII guard since we perform normal restoration now
    g_restore_handler = nullptr;

    RestoreHardware();

    client.Disconnect();
    std::cout << "\nDone! OpenRGB connection closed. Original status restored cleanly.\n" << std::endl;

    return 0;
}
