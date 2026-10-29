/**
 * @file Main.cpp
 * @brief Simple, well-commented example and reference guide for Gorgon::Devices::OpenRGB.
 *
 * This example demonstrates how to accomplish common RGB hardware lighting goals:
 *
 *   Goal 1. Checking availability and enumerating connected RGB devices
 *   Goal 2. Highlighting individual keys and key groups (Base State)
 *   Goal 3. Displaying an interpolated progress bar on keys (e.g., Health / Loading)
 *   Goal 4. Drawing a 2D image or bitmap onto the keyboard matrix
 *   Goal 5. Playing software lighting effects over the Base State
 *   Goal 6. Restoring hardware to the user's original lighting state
 *
 * ============================================================================
 * HOW OPENRGB WORKS WITH SCENES (Gorgon::Scene & Gorgon::Animation::Governor)
 * ============================================================================
 * While this example keeps things simple by using the global `OpenRGB::`
 * functions directly, every `Gorgon::Scene` (and `Animation::Governor`) has a
 * built-in `.OpenRGB` member (`Gorgon::Devices::OpenRGB::Context`) that mirrors
 * the exact same API:
 *
 *   1. Scene-Isolated Lighting:
 *      Instead of calling `OpenRGB::SetKeyGroupColor(...)`, call it on your scene:
 *
 *          combatScene.OpenRGB.SetKeyGroupColor(OpenRGB::KeyGroup::WASD, RGBA::Red);
 *          menuScene.OpenRGB.SetKeyColor(Keycodes::Enter, RGBA::Green);
 *
 *   2. Automatic Switching on Scene Activation:
 *      Whenever a scene becomes active (via `SceneManager` or `scene.Activate()`),
 *      Gorgon automatically switches the physical hardware to that scene's
 *      lighting layer, progressbars, and active effects. Animations belonging to
 *      inactive scenes are automatically paused and hidden.
 *
 *   3. Zero Overhead & Global Fallback:
 *      Creating a `Scene` does NOT initialize OpenRGB or allocate device layers
 *      until you actually call a method on `scene.OpenRGB`. Furthermore, any
 *      device not modified in the active scene automatically falls back to the
 *      Global Layer (the `OpenRGB::` namespace functions shown below).
 *
 *   4. Binding Effects to a Scene:
 *      Pass your `scene` (or `scene.OpenRGB`) as the first argument to any effect
 *      so it only plays while that scene is active:
 *
 *          OpenRGB::PulseEffect lowHpWarning(combatScene, RGBA::Red, 800);
 *          lowHpWarning.Loop();
 * ============================================================================
 */

#include <Gorgon/Devices/OpenRGB.h>
#include <Gorgon/Graphics/Color.h>
#include <Gorgon/Containers/Image.h>
#include <Gorgon/Main.h>

#include <chrono>
#include <iostream>
#include <thread>

namespace OpenRGB  = Gorgon::Devices::OpenRGB;
namespace Keycodes = Gorgon::Input::Keyboard::Keycodes;
using Gorgon::Graphics::RGBA;

/// Helper to pause between steps so you can inspect the physical keyboard.
static void waitForEnter() {
    std::cout << "  -> Press <Enter> to continue...";
    std::cin.get();
    std::cout << "\n";
}

int main() {
    // =========================================================================
    // Goal 1: Zero-Boilerplate Initialization & Device Discovery
    // =========================================================================
    // You do NOT need to manually call `OpenRGB::Initialize()` before using
    // lighting functions: the subsystem connects lazily to 127.0.0.1:6742 on
    // the first lighting call.
    //
    // All OpenRGB calls are fault-tolerant: if the OpenRGB server is not
    // running or no compatible hardware is attached, functions safely no-op
    // so your game logic never needs `if(OpenRGB::IsAvailable())` guards.
    std::cout << "=== Goal 1: Device Discovery ===\n";

    if (OpenRGB::IsAvailable()) {
        auto devices = OpenRGB::GetDevices();
        std::cout << "Connected to OpenRGB server. Found " << devices.size() << " device(s):\n";
        for (const auto &dev : devices) {
            std::cout << "  [" << dev.GetID() << "] " << dev.GetName()
                      << " (" << dev.GetLEDCount() << " LEDs, "
                      << dev.GetWidth() << "x" << dev.GetHeight() << " matrix, "
                      << dev.GetZoneCount() << " zone(s), initial mode: "
                      << dev.GetActiveModeName() << ")\n";
        }
    } else {
        std::cout << "OpenRGB SDK server is not running on 127.0.0.1:6742.\n"
                  << "All lighting calls below will safely no-op.\n";
    }
    waitForEnter();


    // =========================================================================
    // Goal 2: Highlighting Keys & Key Groups (Base State)
    // =========================================================================
    // Setting key colors modifies the "Base State" of the keyboard.
    // The Base State persists underneath temporary progressbars and animations.
    //
    // Built-in KeyGroup presets:
    //   - OpenRGB::KeyGroup::WASD          (W, A, S, D)
    //   - OpenRGB::KeyGroup::Arrows        (Up, Down, Left, Right)
    //   - OpenRGB::KeyGroup::NumberRow     (1, 2, 3, 4, 5, 6, 7, 8, 9, 0)
    //   - OpenRGB::KeyGroup::FunctionKeys  (F1 through F12)
    //   - OpenRGB::KeyGroup::Modifiers     (Shift, Ctrl, Alt, Super/Windows)
    //   - OpenRGB::KeyGroup::Numpad        (All numpad keys)
    //   - OpenRGB::KeyGroup::NumpadDigits  (Numpad 0 through 9)
    //   - OpenRGB::KeyGroup::Letters       (A through Z)
    //   - OpenRGB::KeyGroup::All           (Every mapped key on the keyboard)
    std::cout << "=== Goal 2: Highlighting Keys & Key Groups ===\n";
    std::cout << "Lighting Escape (Red), WASD (Green), Space (Cyan), and Q/E/R abilities (Gold).\n";

    // Start with a clean unlit base state across all devices
    OpenRGB::ClearAll();

    // Highlight individual keys using standard Gorgon Keycodes
    OpenRGB::SetKeyColor(Keycodes::Escape, RGBA{255, 0, 0, 255});
    OpenRGB::SetKeyColor(Keycodes::Space,  RGBA{0, 220, 255, 255});

    // Highlight a built-in key group
    OpenRGB::SetKeyGroupColor(OpenRGB::KeyGroup::WASD, RGBA{0, 255, 80, 255});

    // Define and highlight a custom key group (e.g., ability hotkeys Q, E, R)
    OpenRGB::KeyGroup abilityKeys{{Keycodes::Q, Keycodes::E, Keycodes::R}};
    OpenRGB::SetKeyGroupColor(abilityKeys, RGBA{255, 180, 0, 255});

    waitForEnter();


    // =========================================================================
    // Goal 3: Displaying a Progress Bar on Keys or LED Strips (Health / Loading)
    // =========================================================================
    // `OpenRGB::Progressbar` implements `Gorgon::ProgressDisplay` and renders a
    // smoothly interpolated bar over any ordered `KeyGroup`, linear `Zone`, or
    // LED Strip `Device` (treated as a `width x 1` matrix).
    //
    // Common targets:
    //   - OpenRGB::KeyGroup::NumberRow     (10 keys: '1' through '0')
    //   - OpenRGB::KeyGroup::FunctionKeys  (12 keys: 'F1' through 'F12')
    //   - OpenRGB::KeyGroup::NumpadDigits  (10 keys: 'Numpad_0' through 'Numpad_9')
    //   - Any LED Strip Device or Zone     (e.g., `OpenRGB::CreateProgressbar(*strip)`)
    //
    // You can also attach level-color rules so the bar changes color based on
    // its current ratio (e.g., Red when critical, Yellow when medium, Green when high).
    std::cout << "=== Goal 3: Progress Bar (Keyboard Number Row & LED Strips) ===\n";
    std::cout << "Animating a health/loading bar across the Number Row (and any connected LED strip)...\n";

    auto healthBar = OpenRGB::CreateProgressbar(OpenRGB::KeyGroup::NumberRow);
    OpenRGB::Progressbar stripBar;
    stripBar.TargetAllStrips();

    // Optional: change bar color automatically depending on the progress value
    std::vector<OpenRGB::Progressbar::LevelColor> levels = {
        {{0.0f, 0.35f}, RGBA{255, 30, 0, 255}},   // Critical: Red
        {{0.35f, 0.70f}, RGBA{255, 200, 0, 255}}, // Warning:  Yellow
        {{0.70f, 1.0f}, RGBA{0, 255, 80, 255}}    // Healthy:  Green
    };
    healthBar.SetLevelColors(levels);
    stripBar.SetLevelColors(levels);

    // Note: Call `healthBar.SetSingleKeyMode(true)` if you only want a single
    // sliding indicator dot instead of filling all keys/LEDs up to the value.

    for (int i = 0; i <= 40; ++i) {
        float ratio = static_cast<float>(i) / 40.0f;
        healthBar.SetProgressRatio(ratio);
        stripBar.SetProgressRatio(ratio);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    waitForEnter();

    // Clearing the progressbar removes its overlay while keeping the Base State intact
    healthBar.Clear();
    stripBar.Clear();


    // =========================================================================
    // Goal 4: Drawing a 2D Image / Bitmap onto the Keyboard Matrix
    // =========================================================================
    // Keyboards (and LED matrix panels) expose a 2D coordinate grid. You can draw
    // any `Gorgon::Graphics::Bitmap` or `Gorgon::Containers::Image` onto the
    // physical keys using:
    //   - `OpenRGB::Draw(image, offset, blend)`       -- 1:1 pixel mapping
    //   - `OpenRGB::DrawScaled(image, interp, blend)` -- stretches to fit the matrix
    //
    // Supported BlendModes:
    //   - BlendMode::Overwrite   (replaces underlying base colors)
    //   - BlendMode::AlphaBlend  (blends using the image's alpha channel)
    //   - BlendMode::Additive    (adds RGB values onto existing colors)
    //   - BlendMode::Multiply    (multiplies existing colors by image colors)
    std::cout << "=== Goal 4: Drawing a 2D Image onto the Keyboard Matrix ===\n";
    std::cout << "Drawing a horizontal sunset gradient image across the keyboard matrix,\n"
              << "then re-highlighting WASD on top.\n";

    Gorgon::Containers::Image gradient({24, 6}, Gorgon::Graphics::ColorMode::RGBA);
    for (int y = 0; y < gradient.GetHeight(); ++y) {
        for (int x = 0; x < gradient.GetWidth(); ++x) {
            float t = static_cast<float>(x) / static_cast<float>(gradient.GetWidth() - 1);
            gradient(x, y, 0) = static_cast<Gorgon::Byte>(255 * (1.0f - t)); // Red
            gradient(x, y, 1) = static_cast<Gorgon::Byte>(60 * (1.0f - t));  // Green
            gradient(x, y, 2) = static_cast<Gorgon::Byte>(255 * t);          // Blue
            gradient(x, y, 3) = 255;                                         // Alpha
        }
    }

    // Stretch the 2D image across all matrix zones in the Base State
    OpenRGB::DrawScaled(gradient);

    // Highlight WASD in bright green over the gradient
    OpenRGB::SetKeyGroupColor(OpenRGB::KeyGroup::WASD, RGBA{0, 255, 100, 255});

    waitForEnter();


    // =========================================================================
    // Goal 5: Playing Software Lighting Effects Over the Base State
    // =========================================================================
    // Software effects inherit from `OpenRGB::Animation` (and `Gorgon::Animation::Base`).
    // They have value semantics, can be allocated directly on the stack or as class
    // members, and composite non-destructively over your Base State. When a one-shot
    // effect finishes, the keyboard automatically returns to its Base State.
    //
    // Below we demonstrate `ExplosionEffect`. Other built-in effects include:
    //
    //   1. OpenRGB::FadeEffect(startColor, endColor, durationMs, blendMode)
    //      Smoothly interpolates target LEDs between two colors (or across multiple
    //      color stops via `.SetColorStops({...})`). Great for damage flashes or death fades.
    //
    //   2. OpenRGB::PulseEffect(peakColor, periodMs, troughColor, blendMode)
    //      Smooth sine-wave breathing between two colors. Call `.Loop()` for continuous
    //      low-health or active-buff pulsing.
    //
    //   3. OpenRGB::BlinkEffect(onColor, periodMs, dutyCycle, offColor, blendMode)
    //      Alternates between on and off colors with a configurable duty cycle (0.0 - 1.0).
    //
    //   4. OpenRGB::SwipeEffect(color, SwipeDirection, durationMs, bandWidth, blendMode)
    //      Sweeps a band of light across the keyboard (`LeftToRight`, `RightToLeft`,
    //      `TopToBottom`, `BottomToTop`). Great for level-up or buff pickup indicators.
    //
    //   5. OpenRGB::ReactiveEffect(color, fadeMs, blendMode)
    //      Illuminates keys as they are pressed and fades them back to the Base State.
    //      Supports radial neighbor ripples (`.SetRippleRadius(1.8f)`), rainbow random
    //      colors (`.SetRandomColor(true)`), and automatic window input hooking
    //      (`.AttachToWindow(window)`).
    //
    // Targeting & Chaining:
    //   - Effects target all devices by default, or can be restricted via:
    //     `.TargetKeyGroup(OpenRGB::KeyGroup::WASD)`, `.TargetKey(Keycodes::Space)`,
    //     `.TargetDevice(dev)`, or `.TargetZone(zone)`.
    //   - Chain animations sequentially using `effect.FinishedEvent.Register(...)`.
    std::cout << "=== Goal 5: Software Lighting Effect (ExplosionEffect) ===\n";
    std::cout << "Playing a radial orange shockwave from the center of the keyboard.\n"
              << "Notice how the gradient and WASD Base State remain intact underneath!\n";

    {
        OpenRGB::ExplosionEffect explosion(RGBA{255, 160, 0, 255}, 950, 3.5f);

        // Optional: center the explosion on a specific key instead of the matrix center:
        // explosion.SetOriginKey(Keycodes::G);

        // Reset frame delta timer before starting the animation loop
        Gorgon::NextFrame(false);
        explosion.Play();

        // In a regular windowed Gorgon app, `window.Run()` or your main loop calls
        // `Gorgon::NextFrame()` automatically every frame. Here in a console app,
        // we pump `Gorgon::NextFrame(false)` for ~1 second while the effect plays:
        while (!explosion.IsFinished()) {
            Gorgon::NextFrame(false);
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    std::cout << "  Explosion finished — keyboard returned to its Base State.\n";
    waitForEnter();


    // =========================================================================
    // Goal 6: Scene Integration Summary & Automatic Hardware Restoration
    // =========================================================================
    std::cout << "=== Goal 6: Scenes & Hardware Restoration ===\n";
    std::cout << "Tip: In multi-scene games, use `myScene.OpenRGB.SetKeyGroupColor(...)`\n"
              << "     instead of global `OpenRGB::` calls. Gorgon will automatically\n"
              << "     switch keyboard lighting and pause/resume effects as scenes change.\n\n";

    // You can manually revert all devices back to the user's original hardware
    // mode and colors at any time by calling `OpenRGB::RestoreAll()`.
    // Even if you don't call it explicitly, Gorgon automatically restores all
    // hardware when the application exits.
    std::cout << "Restoring hardware to its original lighting state and exiting...\n";
    OpenRGB::RestoreAll();

    return 0;
}
