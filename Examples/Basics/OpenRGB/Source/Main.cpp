/**
 * @file Main.cpp
 * @brief Interactive, well-commented example and reference guide for Gorgon::Devices::OpenRGB
 *        and Gorgon::Input::Shortcuts.
 *
 * This example launches an interactive UI window with separate buttons and descriptions
 * for each goal, demonstrating how to accomplish common RGB hardware lighting goals:
 *
 *   Goal 1. Checking availability and enumerating connected RGB devices
 *   Goal 2. Highlighting individual keys and key groups (Base State)
 *   Goal 3. Displaying a health progress bar on number keys with +/- adjustment
 *   Goal 4. Displaying a progress bar on LED strips (e.g. ring/ambient) with +/- adjustment
 *   Goal 5. Drawing a 2D image or bitmap onto the keyboard matrix
 *   Goal 6. Playing software lighting effects over the Base State
 *   Goal 7. Modifier-aware shortcuts & dynamic action key lighting
 *   Goal 8. Focus lost & temporary force restore (Suspend / Resume / StateSnapshot)
 *   Goal 9. Restoring hardware to the user's original OEM lighting state
 *
 * ============================================================================
 * HOW OPENRGB & SHORTCUTS WORK WITH SCENES (Gorgon::Scene)
 * ============================================================================
 * While this example operates on the active window and global OpenRGB layer,
 * every `Gorgon::Scene` (and `Animation::Governor`) has built-in `.OpenRGB` and
 * `.Shortcuts` members that mirror the exact same API:
 *
 *   1. Scene-Isolated Lighting & Shortcuts:
 *      Instead of calling `OpenRGB::SetKeyGroupColor(...)` or `window.Shortcuts.Set(...)`,
 *      call them on your scene:
 *
 *          combatScene.OpenRGB.SetKeyGroupColor(OpenRGB::KeyGroup::WASD, RGBA{255, 0, 0, 255});
 *          combatScene.Shortcuts.Set(Keycodes::P, [&]() { pauseGame(); }, RGBA{255, 255, 0, 255});
 *
 *   2. Automatic Switching on Scene Activation:
 *      Whenever a scene becomes active (via `SceneManager::SwitchScene()` or `scene.Activate()`),
 *      Gorgon automatically activates that scene's shortcuts on the window and switches
 *      the physical hardware to that scene's lighting layer. When the scene deactivates,
 *      its shortcuts unhook and its lighting smoothly yields to the next scene or base state.
 *
 *   3. Zero Overhead & Global Fallback:
 *      Creating a `Scene` does NOT initialize OpenRGB or allocate device layers until you
 *      actually call a method on `scene.OpenRGB`. Any device not modified in the active
 *      scene automatically falls back to the Global Layer.
 * ============================================================================
 */

#include <Gorgon/Devices/OpenRGB.h>
#include <Gorgon/Graphics/Color.h>
#include <Gorgon/Containers/Image.h>
#include <Gorgon/Main.h>
#include <Gorgon/UI.h>
#include <Gorgon/UI/Window.h>
#include <Gorgon/Widgets/Button.h>
#include <Gorgon/Widgets/Label.h>
#include <Gorgon/Input/Shortcuts.h>
#include <Gorgon/Input/Keyboard.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace OpenRGB  = Gorgon::Devices::OpenRGB;
namespace Keycodes = Gorgon::Input::Keyboard::Keycodes;
using Modifier     = Gorgon::Input::Keyboard::Modifier;
using Gorgon::Graphics::RGBA;
using Gorgon::UI::Units;
using Gorgon::UI::Percentage;

// -----------------------------------------------------------------------------
// Goal 1: Device Enumeration & Availability
// -----------------------------------------------------------------------------
// Checks connection to the OpenRGB SDK server and enumerates connected devices
// (keyboards, LED strips, mice, motherboards, RAM, etc.).
void Goal1_DeviceEnumeration(Gorgon::Widgets::Label &statusLbl) {
    if(!OpenRGB::IsAvailable()) {
        statusLbl.SetText("Status: OpenRGB server not detected at 127.0.0.1:6742.");
        std::cout << "[Stage 1] OpenRGB server not running.\n";
        return;
    }

    auto keyboards = OpenRGB::GetKeyboards();
    auto strips    = OpenRGB::GetStrips();
    auto allDevs   = OpenRGB::GetDevices();

    std::string msg = "Status: Found " + std::to_string(allDevs.size()) + " device(s) — " +
                      std::to_string(keyboards.size()) + " keyboard(s), " +
                      std::to_string(strips.size()) + " strip(s).";
    statusLbl.SetText(msg);

    std::cout << "[Stage 1] Connected to OpenRGB:\n";
    for(const auto &d : allDevs) {
        std::cout << "  - " << d.GetName() << " (" << d.GetLEDCount() << " LEDs)\n";
    }
}

// -----------------------------------------------------------------------------
// Goal 2: Base State Lighting (Key Groups & Strips)
// -----------------------------------------------------------------------------
// Highlights individual keys and standard groups (WASD, Arrows, Space, Enter)
// and sets ambient lighting on strips. This establishes the "Base State" which
// animations and progressbars overlay without destroying the underlying colors.
void Goal2_BaseStateLighting(Gorgon::Widgets::Label &statusLbl) {
    // 1. WASD movement keys in bright red
    OpenRGB::SetKeyGroupColor(OpenRGB::KeyGroup::WASD, RGBA{255, 30, 30, 255});

    // 2. Arrow keys in sky blue
    OpenRGB::SetKeyGroupColor(OpenRGB::KeyGroup::Arrows, RGBA{30, 140, 255, 255});

    // 3. Enter key in vivid green
    OpenRGB::SetKeyColor(Keycodes::Enter, RGBA{30, 255, 60, 255});

    // 4. Space bar in radiant gold
    OpenRGB::SetKeyColor(Keycodes::Space, RGBA{255, 215, 0, 255});

    // 5. Ambient violet for all connected LED strips
    for(auto &strip : OpenRGB::GetStrips()) {
        strip.SetColor(RGBA{140, 30, 255, 255});
    }

    statusLbl.SetText("Status: Base State applied (WASD=Red, Arrows=Blue, Enter=Green, Space=Gold, Strips=Violet).");
    std::cout << "[Stage 2] Applied static Base State key highlights and strip lighting.\n";
}

// -----------------------------------------------------------------------------
// Goal 3: Keyboard Health / Progress Bar
// -----------------------------------------------------------------------------
// Demonstrates Gorgon::Devices::OpenRGB::Progressbar across keyboard keys (number row 1-0).
// Level colors transition automatically: Green (>50%), Yellow (25-50%), Red (<25%).
// Unfilled LEDs preserve the Base State underneath.
static float s_healthRatio = 1.0f;
static int s_healthStep = 0;

void SetHealthRatio(float ratio, Gorgon::Widgets::Label &statusLbl) {
    static bool initialized = false;
    
    // Create a progress bar targeting the number row keys (1-0)
    static OpenRGB::Progressbar bar{OpenRGB::KeyGroup::NumberRow};

    if(!initialized) {
        // Define level colors for the progress bar, this can be used to mark health levels
        bar.AddLevelColor(Gorgon::Range<Gorgon::Float>(0.50f, 1.00f), RGBA{30, 255, 60, 255});
        bar.AddLevelColor(Gorgon::Range<Gorgon::Float>(0.25f, 0.50f), RGBA{255, 220, 0, 255});
        bar.AddLevelColor(Gorgon::Range<Gorgon::Float>(0.00f, 0.25f), RGBA{255, 30, 0, 255});
        bar.SetBackgroundColor(RGBA{0, 0, 0, 0}); // transparent unfilled background
        initialized = true;
    }

    s_healthRatio = Gorgon::Clamp(ratio, 0.0f, 1.0f);
    bar.SetProgressRatio(s_healthRatio);

    int pct = int(s_healthRatio * 100);
    std::string colorName = (pct >= 50) ? "Green" : ((pct >= 25) ? "Yellow" : "Red");
    statusLbl.SetText("Status: Keyboard Health Progressbar set to " + std::to_string(pct) + "% (" + colorName + ").");
    std::cout << "[Stage 3] Keyboard health set to " << pct << "% (" << colorName << ").\n";
}

void Goal3_HealthProgressbar(Gorgon::Widgets::Label &statusLbl) {
    static const float steps[] = {1.0f, 0.75f, 0.50f, 0.25f, 0.10f};
    s_healthStep = (s_healthStep + 1) % 5;
    SetHealthRatio(steps[s_healthStep], statusLbl);
}

// -----------------------------------------------------------------------------
// Goal 4: LED Strip Progress Bar (e.g. Mana / Ring / Shield)
// -----------------------------------------------------------------------------
// Demonstrates OpenRGB::Progressbar on connected LED strips (e.g., ring strip).
// Shows smooth progression with automatic level transitions:
// Cyan (>50%), Blue (25-50%), Purple (<25%).
static float s_stripRatio = 1.0f;
static int s_stripStep = 0;

void SetStripRatio(float ratio, Gorgon::Widgets::Label &statusLbl) {
    static bool initialized = false;
    static OpenRGB::Progressbar bar;

    // Initialize the strip progress bar only once, can be handled in the constructor if
    // in a class, but here we do it in a function
    if(!initialized) {
        // Target all connected LED strips for the progress bar
        bar.TargetAllStrips();
        // Define level colors for the progress bar, this can be used to mark health levels
        bar.AddLevelColor(Gorgon::Range<Gorgon::Float>(0.50f, 1.00f), RGBA{0, 240, 255, 255});
        bar.AddLevelColor(Gorgon::Range<Gorgon::Float>(0.25f, 0.50f), RGBA{30, 100, 255, 255});
        bar.AddLevelColor(Gorgon::Range<Gorgon::Float>(0.00f, 0.25f), RGBA{180, 0, 255, 255});
        bar.SetBackgroundColor(RGBA{0, 0, 0, 0}); // transparent unfilled background
        initialized = true;
    }

    // Clamp the ratio to [0.0, 1.0] and update the progress bar
    s_stripRatio = Gorgon::Clamp(ratio, 0.0f, 1.0f);
    bar.SetProgressRatio(s_stripRatio);

    int pct = int(s_stripRatio * 100);
    std::string colorName = (pct >= 50) ? "Cyan" : ((pct >= 25) ? "Blue" : "Purple");
    statusLbl.SetText("Status: LED Strip Progressbar set to " + std::to_string(pct) + "% (" + colorName + ").");
    std::cout << "[Stage 4] LED strip progress set to " << pct << "% (" << colorName << ").\n";
}

void Goal4_StripProgressbar(Gorgon::Widgets::Label &statusLbl) {
    static const float steps[] = {1.0f, 0.80f, 0.60f, 0.40f, 0.20f};
    s_stripStep = (s_stripStep + 1) % 5;
    SetStripRatio(steps[s_stripStep], statusLbl);
}

// -----------------------------------------------------------------------------
// Goal 5: 2D Matrix Gradient Drawing
// -----------------------------------------------------------------------------
// Creates a 2D image with a sunset gradient and draws it scaled across the
// physical 2D keyboard matrix and strips using OpenRGB::DrawScaled.
void Goal5_MatrixGradientDrawing(Gorgon::Widgets::Label &statusLbl) {
    constexpr int imgW = 24, imgH = 6;
    Gorgon::Containers::Image gradient(Gorgon::Geometry::Size{imgW, imgH}, Gorgon::Graphics::ColorMode::RGBA);

    for(int y = 0; y < imgH; y++) {
        for(int x = 0; x < imgW; x++) {
            float tx = float(x) / float(imgW - 1);
            float ty = float(y) / float(imgH - 1);

            Gorgon::Byte r = Gorgon::Byte(255 * (1.0f - ty * 0.4f));
            Gorgon::Byte g = Gorgon::Byte(180 * tx * (1.0f - ty));
            Gorgon::Byte b = Gorgon::Byte(255 * ty);

            gradient(x, y, 0) = r;
            gradient(x, y, 1) = g;
            gradient(x, y, 2) = b;
            gradient(x, y, 3) = 255;
        }
    }

    OpenRGB::DrawScaled(gradient);

    statusLbl.SetText("Status: 2D Sunset gradient drawn across keyboard matrix and strips.");
    std::cout << "[Stage 5] Drew 24x6 sunset gradient across keyboard matrix.\n";
}

// -----------------------------------------------------------------------------
// Goal 6: Software Lighting Effect (Explosion / Shockwave)
// -----------------------------------------------------------------------------
// Software animations inherit from `OpenRGB::Animation` and run non-destructively
// over the Base State. When the shockwave finishes, the Base State is automatically restored.
//
// Other built-in effects available in Gorgon::Devices::OpenRGB:
//   - OpenRGB::FadeEffect      : Smooth linear transition between colors or alpha
//   - OpenRGB::PulseEffect     : Rhythmic pulsing/breathing between min and max intensities
//   - OpenRGB::BlinkEffect     : Crisp blinking/strobing at specified frequency
//   - OpenRGB::SwipeEffect     : Directional linear wave moving across zones or keys
//   - OpenRGB::ReactiveEffect  : Dynamic ripple/flash radiating outward from any pressed key
static std::unique_ptr<OpenRGB::ExplosionEffect> s_activeExplosion;

void Goal6_SoftwareLightingEffect(Gorgon::Widgets::Label &statusLbl) {
    s_activeExplosion = std::make_unique<OpenRGB::ExplosionEffect>(RGBA{255, 160, 0, 255}, 900, 3.5f);
    s_activeExplosion->Play();

    statusLbl.SetText("Status: Playing ExplosionEffect shockwave! Base State remains intact underneath.");
    std::cout << "[Stage 6] Triggered software ExplosionEffect shockwave.\n";
}

// -----------------------------------------------------------------------------
// Goal 7: Modifier-Aware Shortcuts & Dynamic Action Key Lighting
// -----------------------------------------------------------------------------
// Uses `Gorgon::Input::Shortcuts` attached to the window (`window.Shortcuts`).
// In ActiveModifiers mode (default):
//   - Holding Ctrl illuminates the physical Ctrl key in electric blue and lights
//     S (Green) and O (Cyan) on the physical keyboard.
//   - Holding Alt illuminates Alt and lights X (Orange).
//   - Holding a modifier with NO registered keys (e.g. Shift alone or Meta) does NOT highlight it!
//   - Pressing the shortcut combination triggers the action!
static bool s_shortcutsSetup = false;
static int s_shortcutModeIdx = 0;

void Goal7_ModifierAwareShortcuts(Gorgon::UI::Window &window, Gorgon::Widgets::Label &statusLbl) {
    if(!s_shortcutsSetup) {
        s_shortcutsSetup = true;

        // Register shortcuts with lambdas and RGB colors:
        // 1. Ctrl + S -> Save (Green)
        window.Shortcuts.Set(Keycodes::S, Modifier::Ctrl, [&statusLbl]() {
            statusLbl.SetText("Executed Shortcut: [Ctrl + S] -> Document Saved successfully!");
            std::cout << "  -> Shortcut triggered: Ctrl + S (Save)\n";
        }, RGBA{0, 255, 60, 255});

        // 2. Ctrl + O -> Open (Cyan)
        window.Shortcuts.Set(Keycodes::O, Modifier::Ctrl, [&statusLbl]() {
            statusLbl.SetText("Executed Shortcut: [Ctrl + O] -> File Open dialog launched!");
            std::cout << "  -> Shortcut triggered: Ctrl + O (Open)\n";
        }, RGBA{0, 200, 255, 255});

        // 3. Ctrl + Shift + F -> Find (Yellow)
        window.Shortcuts.Set(Keycodes::F, Modifier::ShiftCtrl, [&statusLbl]() {
            statusLbl.SetText("Executed Shortcut: [Ctrl + Shift + F] -> Find in Project launched!");
            std::cout << "  -> Shortcut triggered: Ctrl + Shift + F (Find)\n";
        }, RGBA{255, 220, 0, 255});

        // 4. Alt + X -> Exit (Orange)
        window.Shortcuts.Set(Keycodes::X, Modifier::Alt, [&statusLbl, &window]() {
            statusLbl.SetText("Executed Shortcut: [Alt + X] -> Exit shortcut activated. Goodbye!");
            std::cout << "  -> Shortcut triggered: Alt + X (Exit)\n";
            window.Quit();
        }, RGBA{255, 100, 0, 255});

        // 5. F1 -> Help (Purple)
        window.Shortcuts.Set(Keycodes::F1, Modifier::None, [&statusLbl]() {
            statusLbl.SetText("Executed Shortcut: [F1] -> Help dialog launched!");
            std::cout << "  -> Shortcut triggered: F1 (Help)\n";
        }, RGBA{180, 0, 255, 255});
        
    }

    // Clicking the button cycles the highlight mode between ActiveModifiers and RegularKeys
    s_shortcutModeIdx = (s_shortcutModeIdx + 1) % 2;
    if(s_shortcutModeIdx == 1) {
        window.Shortcuts.SetHighlightMode(Gorgon::Input::ModifierHighlightMode::ActiveModifiers);
        statusLbl.SetText("Shortcuts Mode: ActiveModifiers (default). Hold Ctrl or Alt on your keyboard to illuminate shortcuts!");
        std::cout << "[Stage 7] Mode set to ActiveModifiers. Hold Ctrl or Alt on your physical keyboard!\n";
    }
    else {
        window.Shortcuts.SetHighlightMode(Gorgon::Input::ModifierHighlightMode::RegularKeys);
        statusLbl.SetText("Shortcuts Mode: RegularKeys. All bound shortcut keys and modifiers are constantly illuminated.");
        std::cout << "[Stage 7] Mode set to RegularKeys. All shortcut keys are constantly illuminated.\n";
    }
}

// -----------------------------------------------------------------------------
// Goal 8: Focus Lost & Temporary Force Restore (Suspend / Resume / StateSnapshot)
// -----------------------------------------------------------------------------
// Demonstrates how to restore OEM hardware lighting when window focus is lost,
// when starting the application disabled, or when toggled via a Settings menu:
//   - OpenRGB::Suspend() / ForceRestore(): Can be called before or after initialization.
//     If called at startup (or via OpenRGB::Initialize(true)), OpenRGB starts suspended
//     and never modifies physical hardware while all API calls continue to update software
//     layers in memory. If called while running, it reverts modified devices to their
//     original OEM mode while keeping software layers, animations, and progressbars intact.
//   - OpenRGB::Resume(): Re-engages direct mode and flushes all software colors back to hardware.
//   - OpenRGB::CaptureState() & RestoreState(): Allows capturing/restoring software snapshots.
//
// Recommendation for Games & Applications:
//   Place an "Enable OpenRGB Lighting" toggle in your application's Settings menu.
//   On startup, if the user's saved preference has RGB disabled, call `OpenRGB::Suspend();`
//   (or `OpenRGB::Initialize(true);`). Your game code can unconditionally call `SetKeyColor()`,
//   update progressbars, play effects, and register shortcuts without `if(rgbEnabled)` branches;
//   toggling the setting at runtime simply calls `OpenRGB::Resume()` or `OpenRGB::Suspend()`.
static OpenRGB::StateSnapshot s_savedSnapshot;

void Goal8_FocusAndForceRestore(Gorgon::Widgets::Label &statusLbl) {
    if(OpenRGB::IsSuspended()) {
        OpenRGB::Resume();
        statusLbl.SetText("Status: OpenRGB Resumed. Software RGB lighting restored across all devices.");
        std::cout << "[Stage 8] Resumed software RGB lighting.\n";
    }
    else {
        s_savedSnapshot = OpenRGB::CaptureState();
        OpenRGB::Suspend();
        statusLbl.SetText("Status: OpenRGB Suspended (Force Restored). OEM hardware lighting active; software state preserved.");
        std::cout << "[Stage 8] Force-restored hardware to OEM profile (Suspend).\n";
    }
}

// -----------------------------------------------------------------------------
// Goal 9: Hardware Restoration (Permanent Restore)
// -----------------------------------------------------------------------------
// Reverts all connected devices to the user's initial OEM profiles and colors.
void Goal9_RestoreHardware(Gorgon::Widgets::Label &statusLbl) {
    OpenRGB::RestoreAll();
    statusLbl.SetText("Status: Hardware restored to original OEM profile and colors.");
    std::cout << "[Stage 9] Restored all hardware to initial user state.\n";
}

// =============================================================================
// Application Entry Point
// =============================================================================
int main() {
    Gorgon::Initialize("OpenRGBExample");

    // -------------------------------------------------------------------------
    // Tip: Starting with OpenRGB Suspended (Settings Toggle Pattern)
    // -------------------------------------------------------------------------
    // To start an application with OpenRGB disabled by default (or based on a
    // user configuration setting), call `OpenRGB::Suspend();` or
    // `OpenRGB::Initialize(true);` here before any lighting calls.
    // All OpenRGB calls (SetKeyColor, Progressbar, Shortcuts, Animations) will
    // work normally in memory without modifying physical devices at all until
    // `OpenRGB::Resume()` is called (for example, from a Settings menu toggle).
    //
    // if(!userSettings.enableOpenRGB) {
    //     OpenRGB::Suspend();
    // }

    Gorgon::UI::Window window({980, 840}, "openrgb_example", "Gorgon OpenRGB & Shortcut Keys Interactive Guide", true);
    Gorgon::UI::Initialize();

    window.Center();

    // Title and Subtitle
    Gorgon::Widgets::Label titleLbl("Gorgon OpenRGB & Shortcut Keys Interactive Guide");
    titleLbl.Move(Units(0.8f, 0.5f));
    titleLbl.SetWidth(Percentage(95));
    window.Add(titleLbl);

    Gorgon::Widgets::Label subtitleLbl("Click stage buttons or +/- to adjust; hold Ctrl or Alt on physical keyboard; alt-tab to test focus restore.");
    subtitleLbl.Move(Units(0.8f, 1.4f));
    subtitleLbl.SetWidth(Percentage(95));
    window.Add(subtitleLbl);

    // Status Display Label at the bottom
    Gorgon::Widgets::Label statusLbl("Status: Initialized. Shortcuts are active! Try holding Ctrl or Alt on your keyboard.");
    statusLbl.Move(Units(0.8f, 20.6f));
    statusLbl.SetWidth(Percentage(95));
    window.Add(statusLbl);

    // Stage 1: Device Enumeration
    Gorgon::Widgets::Button btn1("Stage 1: Enumerate Devices", [&]() { Goal1_DeviceEnumeration(statusLbl); });
    btn1.Move(Units(0.8f, 2.4f));
    btn1.SetWidth(Units(8.5f));
    window.Add(btn1);

    Gorgon::Widgets::Label desc1("Queries OpenRGB server and lists connected keyboards, LED strips, and other hardware.");
    desc1.Move(Units(0.8f, 3.4f));
    desc1.SetWidth(Percentage(95));
    window.Add(desc1);

    // Stage 2: Base State Lighting
    Gorgon::Widgets::Button btn2("Stage 2: Static Base State", [&]() { Goal2_BaseStateLighting(statusLbl); });
    btn2.Move(Units(0.8f, 4.4f));
    btn2.SetWidth(Units(8.5f));
    window.Add(btn2);

    Gorgon::Widgets::Label desc2("Sets persistent Base State colors: WASD (Red), Arrows (Blue), Enter (Green), Space (Gold), Strips (Violet).");
    desc2.Move(Units(0.8f, 5.4f));
    desc2.SetWidth(Percentage(95));
    window.Add(desc2);

    // Stage 3: Keyboard Health Progressbar + Buttons
    Gorgon::Widgets::Button btn3("Stage 3: Cycle Health Bar", [&]() { Goal3_HealthProgressbar(statusLbl); });
    btn3.Move(Units(0.8f, 6.4f));
    btn3.SetWidth(Units(8.5f));
    window.Add(btn3);

    Gorgon::Widgets::Button btn3_minus("-", [&]() { SetHealthRatio(s_healthRatio - 0.10f, statusLbl); });
    btn3_minus.Move(Units(9.6f, 6.4f));
    btn3_minus.SetWidth(Units(1.2f));
    window.Add(btn3_minus);

    Gorgon::Widgets::Button btn3_plus("+", [&]() { SetHealthRatio(s_healthRatio + 0.10f, statusLbl); });
    btn3_plus.Move(Units(11.0f, 6.4f));
    btn3_plus.SetWidth(Units(1.2f));
    window.Add(btn3_plus);

    Gorgon::Widgets::Label desc3("Cycles health ratio on number keys (1-0). Use +/- to fine-tune. Colors: Green (>50%), Yellow (25-50%), Red (<25%).");
    desc3.Move(Units(0.8f, 7.4f));
    desc3.SetWidth(Percentage(95));
    window.Add(desc3);

    // Stage 4: LED Strip Progressbar + Buttons
    Gorgon::Widgets::Button btn4("Stage 4: Strip Progress Bar", [&]() { Goal4_StripProgressbar(statusLbl); });
    btn4.Move(Units(0.8f, 8.4f));
    btn4.SetWidth(Units(8.5f));
    window.Add(btn4);

    Gorgon::Widgets::Button btn4_minus("-", [&]() { SetStripRatio(s_stripRatio - 0.10f, statusLbl); });
    btn4_minus.Move(Units(9.6f, 8.4f));
    btn4_minus.SetWidth(Units(1.2f));
    window.Add(btn4_minus);

    Gorgon::Widgets::Button btn4_plus("+", [&]() { SetStripRatio(s_stripRatio + 0.10f, statusLbl); });
    btn4_plus.Move(Units(11.0f, 8.4f));
    btn4_plus.SetWidth(Units(1.2f));
    window.Add(btn4_plus);

    Gorgon::Widgets::Label desc4("Targets connected LED strips (e.g. ring). Use +/- to fine-tune. Colors: Cyan (>50%), Blue (25-50%), Purple (<25%).");
    desc4.Move(Units(0.8f, 9.4f));
    desc4.SetWidth(Percentage(95));
    window.Add(desc4);

    // Stage 5: Matrix Gradient Drawing
    Gorgon::Widgets::Button btn5("Stage 5: 2D Matrix Gradient", [&]() { Goal5_MatrixGradientDrawing(statusLbl); });
    btn5.Move(Units(0.8f, 10.4f));
    btn5.SetWidth(Units(8.5f));
    window.Add(btn5);

    Gorgon::Widgets::Label desc5("Draws a 24x6 sunset gradient image smoothly scaled across physical keyboard matrix and strips.");
    desc5.Move(Units(0.8f, 11.4f));
    desc5.SetWidth(Percentage(95));
    window.Add(desc5);

    // Stage 6: Software Lighting Effect
    Gorgon::Widgets::Button btn6("Stage 6: Shockwave Effect", [&]() { Goal6_SoftwareLightingEffect(statusLbl); });
    btn6.Move(Units(0.8f, 12.4f));
    btn6.SetWidth(Units(8.5f));
    window.Add(btn6);

    Gorgon::Widgets::Label desc6("Plays an ExplosionEffect shockwave over Base State non-destructively; Base State returns when finished.");
    desc6.Move(Units(0.8f, 13.4f));
    desc6.SetWidth(Percentage(95));
    window.Add(desc6);

    // Stage 7: Modifier-Aware Shortcuts
    Gorgon::Widgets::Button btn7("Stage 7: Toggle Shortcut Mode", [&]() { Goal7_ModifierAwareShortcuts(window, statusLbl); });
    btn7.Move(Units(0.8f, 14.4f));
    btn7.SetWidth(Units(8.5f));
    window.Add(btn7);

    Gorgon::Widgets::Label desc7("Binds Ctrl+S (Save), Ctrl+O (Open), Ctrl+Shift+F (Find), Alt+X (Exit). Hold Ctrl or Alt on your physical keyboard!");
    desc7.Move(Units(0.8f, 15.4f));
    desc7.SetWidth(Percentage(95));
    window.Add(desc7);

    // Stage 8: Focus Lost & Force Restore (Suspend / Resume)
    Gorgon::Widgets::Button btn8("Stage 8: Toggle Suspend/Resume", [&]() { Goal8_FocusAndForceRestore(statusLbl); });
    btn8.Move(Units(0.8f, 16.4f));
    btn8.SetWidth(Units(8.5f));
    window.Add(btn8);

    Gorgon::Widgets::Label desc8("Force-restores OEM state (Suspend) while preserving software layers in memory, or resumes. Also triggers on focus lost/gain!");
    desc8.Move(Units(0.8f, 17.4f));
    desc8.SetWidth(Percentage(95));
    window.Add(desc8);

    // Stage 9: Restore OEM Hardware
    Gorgon::Widgets::Button btn9("Stage 9: Restore OEM Lighting", [&]() { Goal9_RestoreHardware(statusLbl); });
    btn9.Move(Units(0.8f, 18.4f));
    btn9.SetWidth(Units(8.5f));
    window.Add(btn9);

    Gorgon::Widgets::Label desc9("Reverts all devices to initial captured OEM profiles and colors without disconnecting.");
    desc9.Move(Units(0.8f, 19.4f));
    desc9.SetWidth(Percentage(95));
    window.Add(desc9);

    // Bottom Action Buttons: Suspend, Resume, Restore OEM, and Exit
    Gorgon::Widgets::Button suspendBtn("Suspend (OEM)", [&]() {
        OpenRGB::Suspend();
        statusLbl.SetText("OpenRGB Suspended: Hardware temporarily restored to OEM state.");
    });
    suspendBtn.Move(Units(11.0f, 21.6f));
    suspendBtn.SetWidth(Units(4.6f));
    window.Add(suspendBtn);

    Gorgon::Widgets::Button resumeBtn("Resume RGB", [&]() {
        OpenRGB::Resume();
        statusLbl.SetText("OpenRGB Resumed: Software RGB lighting restored across all devices.");
    });
    resumeBtn.Move(Units(16.0f, 21.6f));
    resumeBtn.SetWidth(Units(4.6f));
    window.Add(resumeBtn);

    Gorgon::Widgets::Button restoreBtn("Restore OEM", [&]() {
        Goal9_RestoreHardware(statusLbl);
    });
    restoreBtn.Move(Units(21.0f, 21.6f));
    restoreBtn.SetWidth(Units(4.6f));
    window.Add(restoreBtn);

    Gorgon::Widgets::Button quitBtn("Exit Example", [&]() {
        window.Quit();
    });
    quitBtn.Move(Units(26.0f, 21.6f));
    quitBtn.SetWidth(Units(4.6f));
    window.Add(quitBtn);

    // Pre-register shortcuts so holding Ctrl/Alt works immediately
    Goal7_ModifierAwareShortcuts(window, statusLbl);

    // -------------------------------------------------------------------------
    // Automatic Restoration on Window Focus Lost
    // -------------------------------------------------------------------------
    // When the application window loses focus (e.g. user alt-tabs away),
    // OpenRGB::Suspend() temporarily restores physical hardware to original OEM
    // lighting while preserving all software state in memory. When focus returns,
    // OpenRGB::Resume() instantly restores all software RGB lighting!
    window.LostFocusEvent.Register([&]() {
        OpenRGB::Suspend();
        statusLbl.SetText("Window Lost Focus: Hardware temporarily restored to OEM profile (OpenRGB::Suspend).");
        std::cout << "[Focus] Window lost focus: hardware temporarily restored to OEM state.\n";
    });

    window.FocusedEvent.Register([&]() {
        OpenRGB::Resume();
        statusLbl.SetText("Window Focused: Resumed software RGB lighting (OpenRGB::Resume).");
        std::cout << "[Focus] Window regained focus: resumed software RGB lighting.\n";
    });

    // Clean exit on Escape or close button
    window.ClosingEvent.Register([&]() {
        window.Quit();
    });

    window.KeyEvent.Register([&](Gorgon::Input::Key key, float state) {
        if(!state && key == Keycodes::Escape) {
            window.Quit();
            return true;
        }
        return false;
    });

    std::cout << "========================================================\n"
              << " Gorgon OpenRGB & Shortcut Keys Interactive Guide Running\n"
              << "========================================================\n"
              << " Window opened. Try holding Ctrl or Alt on your keyboard!\n"
              << " Alt-tab to test automatic OEM restore on window focus lost.\n"
              << " Shortcuts bound:\n"
              << "   - Ctrl + S         -> Save (Green key)\n"
              << "   - Ctrl + O         -> Open (Cyan key)\n"
              << "   - Ctrl + Shift + F -> Find (Yellow key)\n"
              << "   - Alt + X          -> Exit (Orange key)\n"
              << " Press Escape or click Exit to quit.\n";

    window.Run();
    return 0;
}
