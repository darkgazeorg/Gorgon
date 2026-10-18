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
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <thread>
#include <chrono>

namespace {

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
            } else if (target == "space" && (led_lower.find("space") != std::string::npos || led_lower == "key: spacebar")) {
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

    openrgb::Client client;

    std::cout << "\n[1/6] Connecting to OpenRGB server (127.0.0.1:6742)..." << std::endl;
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

    std::cout << "\n[2/6] Enumerating connected RGB controllers..." << std::endl;
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

    std::cout << "\n[3/6] Target Keyboard Selected:" << std::endl;
    std::cout << "    Controller: [" << keyboard_id << "] \"" << kb_ctrl.name << "\"" << std::endl;
    std::cout << "    Escape LED: [" << esc_led_idx << "] \"" << kb_ctrl.leds[esc_led_idx].name << "\"" << std::endl;

    // Backup current state
    int32_t original_mode_idx = kb_ctrl.active_mode;
    std::string original_mode_name = (original_mode_idx >= 0 && original_mode_idx < static_cast<int>(kb_ctrl.modes.size()))
                                     ? kb_ctrl.modes[original_mode_idx].name : "N/A";
    openrgb::Mode original_mode = (original_mode_idx >= 0 && original_mode_idx < static_cast<int>(kb_ctrl.modes.size()))
                                  ? kb_ctrl.modes[original_mode_idx] : openrgb::Mode{};
    std::vector<openrgb::Color> original_colors = kb_ctrl.colors;

    bool was_direct = (original_mode_name == "Direct") ||
                      ((original_mode.flags & openrgb::ModeFlags::HasPerLedColor) &&
                       original_mode.color_mode == static_cast<uint32_t>(openrgb::ModeColorMode::PerLed));

    std::cout << "    Original Mode: [" << original_mode_idx << "] \"" << original_mode_name << "\"" << std::endl;
    std::cout << "    Mode Type:     " << (was_direct ? "Direct / Per-LED" : "Hardware Effect (Mode-specific)") << std::endl;
    if (was_direct) {
        std::cout << "    Original Colors captured: " << original_colors.size() << " LEDs" << std::endl;
    }

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
    std::vector<openrgb::Color> black_colors(kb_ctrl.leds.size(), openrgb::Color::FromRGB(0, 0, 0));
    client.UpdateLEDs(static_cast<uint32_t>(keyboard_id), black_colors);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // -------------------------------------------------------------
    // Test Step 1: Single LED write (Escape in Red)
    // -------------------------------------------------------------
    std::cout << "\n[4/6] TEST 1: Setting Escape key LED to RED via UpdateSingleLED..." << std::endl;
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
    std::cout << "\n[5/6] TEST 2: Highlighting WASD + Space via full device write (UpdateLEDs)..." << std::endl;

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

    std::cout << "\n>>> [CHECK 2] Are WASD (Green) and Space (Cyan) illuminated (and Escape off)? Press [Enter] to restore status and exit... <<<" << std::flush;
    std::cin.get();

    // -------------------------------------------------------------
    // Test Step 3: Restore original state
    // -------------------------------------------------------------
    std::cout << "\n[6/6] Restoring original status..." << std::endl;

    if (was_direct) {
        // Original mode was Direct mode: restore per-LED colors
        std::cout << "    Restoring original LED colors (" << original_colors.size() << " LEDs)..." << std::endl;
        bool colors_restored = client.UpdateLEDs(static_cast<uint32_t>(keyboard_id), original_colors);
        if (colors_restored) {
            std::cout << "    [+] LED colors restored successfully." << std::endl;
        } else {
            std::cerr << "    [-] Failed to restore LED colors: " << client.GetLastError() << std::endl;
        }
    } else {
        // Original mode was a hardware effect mode (e.g. Reactive, Breathing, Rainbow Wave)
        // 1. Clear Direct mode LEDs on the hardware so no residual keys stay lit
        std::cout << "    Clearing Direct mode LEDs on hardware..." << std::endl;
        client.UpdateLEDs(static_cast<uint32_t>(keyboard_id), black_colors);

        // 2. Restore hardware effect mode
        if (original_mode_idx >= 0 && original_mode_idx < static_cast<int>(kb_ctrl.modes.size())) {
            std::cout << "    Restoring original hardware mode [" << original_mode_idx << "] \"" << original_mode_name << "\"..." << std::endl;
            bool mode_restored = client.UpdateMode(static_cast<uint32_t>(keyboard_id), original_mode_idx, original_mode);
            if (mode_restored) {
                std::cout << "    [+] Original mode restored successfully." << std::endl;
            } else {
                std::cerr << "    [-] Failed to restore mode: " << client.GetLastError() << std::endl;
            }
        }
    }

    client.Disconnect();
    std::cout << "\nDone! OpenRGB connection closed. Original status restored cleanly.\n" << std::endl;

    return 0;
}
