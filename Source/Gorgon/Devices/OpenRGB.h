#pragma once

/**
 * @file OpenRGB.h
 * @brief High-level hardware lighting subsystem for Gorgon Game Engine.
 *
 * Provides minimum-boilerplate, powerful RGB hardware lighting control:
 * - Zero-boilerplate global API (auto-initializing on first call).
 * - Full 2D LED matrix integration with Gorgon::Graphics::Bitmap and Containers::Image.
 * - Key-specific and key-group illumination via Gorgon::Input::Key.
 * - Automatic background thread with rate-limiting, dirty diffing, and fault-tolerant reconnection.
 * - Automatic hardware state backup and clean restoration on engine shutdown.
 */

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <optional>
#include <functional>

#include "../Graphics/Color.h"
#include "../Geometry/Point.h"
#include "../Geometry/Size.h"
#include "../Containers/Image.h"
#include "../Event.h"
#include "../Input.h"
#include "../Input/Keyboard.h"
#include "../Progress.h"
#include "../Animation.h"
#include "../Input/Shortcuts.h"
#include "../Utils/Logging.h"
#include "OpenRGB/Context.h"

namespace Gorgon {
    class Window;
    namespace Graphics {
        class Bitmap;
    }
}

namespace Gorgon :: Devices :: OpenRGB {

    /// Categories of RGB hardware supported by OpenRGB
    enum class DeviceType {
        Motherboard,
        DRAM,
        GPU,
        Cooler,
        LedStrip,
        Keyboard,
        Mouse,
        MouseMat,
        Headset,
        HeadsetStand,
        Gamepad,
        Light,
        Speaker,
        Virtual,
        Unknown
    };

    /// Physical LED zone layout topology
    enum class ZoneType {
        Single,
        Linear,
        Matrix
    };

    /// Named or custom groups of keyboard keys
    struct KeyGroup {
        std::vector<Input::Key> keys;  ///< Ordered list of keys in the group

        struct ProgressBarTargets {
            static const KeyGroup
                FunctionKeys,
                NumberRow,
                NumpadDigits;
        };

        static const KeyGroup
            WASD,
            Arrows,
            NumberRow,
            FunctionKeys,
            Modifiers,
            Numpad,
            NumpadDigits,
            Letters,
            All;
    };

    /// Blending strategy when drawing a Bitmap, Image, or Effect onto LEDs
    enum class BlendMode {
        Overwrite,       ///< Pixel color directly replaces existing LED color
        AlphaBlend,      ///< Blends pixel over current LED color using the pixel's alpha channel
        Additive,        ///< Adds RGB channels scaled by alpha onto current LED color
        Multiply         ///< Multiplies current LED color by the overlay color
    };

    /// Direction for linear swipe/wave effects
    enum class SwipeDirection {
        LeftToRight,
        RightToLeft,
        TopToBottom,
        BottomToTop
    };

    /// Metadata for a single physical LED
    struct LED {
        int index = 0;
        std::string name;
        Graphics::RGBA color = {0xff000000};
    };

    class Device;
    class KeyboardDevice;
    class Progressbar;
    class Animation;

    /// Internal helper granting the implementation file access to private state.
    struct Access;

    /**
     * @brief Represents an addressable lighting zone on a device (e.g. "Keyboard", "Underglow", "Matrix").
     */
    class Zone {
    public:
        Zone();
        Zone(int index, const std::string &name, ZoneType type, int led_count, int led_offset, const Device &parent);

        /// Returns the zero-based zone index within the parent device
        int GetIndex() const;

        /// Returns the descriptive name of this zone
        const std::string &GetName() const;

        /// Returns the geometry layout type (Single, Linear, Matrix)
        ZoneType GetType() const;

        /// Returns the number of LEDs belonging to this zone
        int GetLEDCount() const;

        /// Returns the starting index of this zone in the parent device's global LED list
        int GetLEDOffset() const;

        /// Returns true if the parent device is currently connected
        bool IsConnected() const;

        /// Returns the parent device this zone belongs to, or nullopt if detached
        std::optional<Device> GetDevice() const;

        /// Returns the governor context this zone handle operates on (nullptr for global layer)
        const Gorgon::Animation::Governor *GetGovernor() const noexcept { return context; }

        // ---------------------------------------------------------
        // Color Controls
        // ---------------------------------------------------------

        /// Returns the current base color of a zone-local LED in this handle's context
        Graphics::RGBA GetLEDColor(int local_idx) const;

        /// Sets the base color of a zone-local LED in this handle's context
        void SetLEDColor(int local_idx, Graphics::RGBA color);

        /// Sets all LEDs in this zone to a solid base color
        void SetColor(Graphics::RGBA color);

        /// Turns off all LEDs in this zone (sets to black)
        void Clear();

        // ---------------------------------------------------------
        // 2D LED Matrix & Bitmap Integration
        // ---------------------------------------------------------

        /// Returns whether this zone is a 2D matrix
        bool IsMatrix() const;

        /// Returns the 2D dimensions (width, height) of the matrix layout (e.g. 32x32, 22x6)
        Geometry::Size GetMatrixSize() const;

        /// Returns the width (columns) of the matrix layout
        int GetWidth() const;

        /// Returns the height (rows) of the matrix layout
        int GetHeight() const;

        /// Returns the zone-local LED index at the given (x, y) coordinate, or -1 if unpopulated
        int GetLEDIndexAt(int x, int y) const;
        int GetLEDIndexAt(Geometry::Point p) const;

        /// Returns the color of the LED at the given matrix coordinate, or transparent black (A=0) if unpopulated/out of bounds
        Graphics::RGBA GetPixel(int x, int y) const;
        Graphics::RGBA GetPixel(Geometry::Point p) const;

        /// Sets the color of the LED at the given matrix coordinate (ignored if unpopulated/out of bounds)
        void SetPixel(int x, int y, Graphics::RGBA color);
        void SetPixel(Geometry::Point p, Graphics::RGBA color);

        /// Draws a Bitmap onto the matrix at the specified top-left offset (1:1 pixel mapping).
        /// Pixels falling outside the matrix boundary or on unpopulated coordinates are clipped.
        void Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset = {0, 0}, BlendMode blend = BlendMode::Overwrite);

        /// Draws image data onto the matrix at the specified top-left offset
        void Draw(const Containers::Image &image, Geometry::Point offset = {0, 0}, BlendMode blend = BlendMode::Overwrite);

        /// Resamples and scales a Bitmap to stretch across the full dimensions of the matrix
        void DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp = Containers::InterpolationMethod::Linear, BlendMode blend = BlendMode::Overwrite);

        /// Resamples and scales image data to stretch across the full dimensions of the matrix
        void DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp = Containers::InterpolationMethod::Linear, BlendMode blend = BlendMode::Overwrite);

        /// Reads back the current base state of this matrix into an Image container.
        /// Populated LEDs have A = 255; missing/unpopulated matrix entries are denoted with A = 0.
        Containers::Image ToImage() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> impl;
        const Gorgon::Animation::Governor *context = nullptr;
        friend class Device;
        friend class KeyboardDevice;
        friend struct Access;
    };

    /**
     * @brief Represents an OpenRGB controller (keyboard, mouse, motherboard, LED strip, etc.).
     */
    class Device {
    public:
        using Mode = int;

        Device();
        virtual ~Device() = default;

        /// Device index assigned by the OpenRGB server
        int GetID() const;

        /// Descriptive name of the device
        const std::string &GetName() const;

        /// Hardware vendor name
        const std::string &GetVendor() const;

        /// Additional description string
        const std::string &GetDescription() const;

        /// Firmware or device version
        const std::string &GetVersion() const;

        /// Serial number if available
        const std::string &GetSerial() const;

        /// Physical connection location string (e.g. "HID: /dev/hidraw3")
        const std::string &GetLocation() const;

        /// Device category
        DeviceType GetType() const;

        /// Returns true if this device is currently physically connected to the server
        bool IsConnected() const;

        /// Returns the governor context this device handle operates on (nullptr for global layer)
        const Gorgon::Animation::Governor *GetGovernor() const noexcept { return context; }

        // ---------------------------------------------------------
        // LEDs & Direct Colors (Across All Zones)
        // ---------------------------------------------------------

        /// Total number of LEDs on this device across all zones
        int GetLEDCount() const;

        /// Returns metadata for all LEDs on this device
        const std::vector<LED> &GetLEDs() const;

        /// Finds an LED by name (case-insensitive search), returns index or -1
        int FindLED(const std::string &name) const;

        /// Gets the current base color of an LED by global device index in this handle's context
        Graphics::RGBA GetLEDColor(int led_idx) const;

        /// Gets the current base color of an LED by name (or transparent black if not found)
        Graphics::RGBA GetLEDColor(const std::string &name) const;

        /// Gets the final composed color (including active progressbars and animations) of an LED by global device index
        Graphics::RGBA GetEffectiveLEDColor(int led_idx) const;

        /// Sets the base color of an LED by global device index in this handle's context
        void SetLEDColor(int led_idx, Graphics::RGBA color);

        /// Sets the base color of an LED by name
        void SetLEDColor(const std::string &name, Graphics::RGBA color);

        /// Sets all LEDs across the entire device to a solid base color
        void SetColor(Graphics::RGBA color);

        /// Sets all LEDs from a color array
        void SetColors(const std::vector<Graphics::RGBA> &colors);

        /// Turns off all LEDs (sets to black)
        void Clear();

        // ---------------------------------------------------------
        // Zones
        // ---------------------------------------------------------

        /// Number of addressable lighting zones on this device
        int GetZoneCount() const;

        /// Gets a zone by index, throws std::out_of_range if index is invalid
        Zone &GetZone(int index);
        const Zone &GetZone(int index) const;

        /// Returns true if this device has a zone with the given name (case-insensitive)
        bool HasZone(const std::string &name) const;

        /// Finds a zone by name, or nullopt
        std::optional<Zone> FindZone(const std::string &name) const;

        /// Returns all zones belonging to this device (bound to this device handle's context)
        std::vector<Zone> GetZones() const;

        // ---------------------------------------------------------
        // Hardware Modes
        // ---------------------------------------------------------

        int GetModeCount() const;
        std::string GetModeName(Mode mode_index) const;
        std::vector<std::string> GetModeNames() const;
        Mode GetActiveMode() const;
        std::string GetActiveModeName() const;
        bool IsDirectMode() const;
        void SetMode(Mode mode_index);
        void SetMode(const std::string &mode_name);

        /// Returns the number of mode-specific colors currently configured on the active mode
        int GetModeColorCount() const;

        /// Returns the minimum number of mode-specific colors supported by the active mode (0 if none)
        int GetModeMinColors() const;

        /// Returns the maximum number of mode-specific colors supported by the active mode (0 if none)
        int GetModeMaxColors() const;

        /// Returns the mode-specific colors of the active mode
        std::vector<Graphics::RGBA> GetModeColors() const;

        /// Returns a specific mode color of the active mode by index
        Graphics::RGBA GetModeColor(int color_index = 0) const;

        /// Sets a specific mode color on the active mode and applies it to the hardware
        void SetModeColor(Graphics::RGBA color, int color_index = 0);

        /// Sets all mode-specific colors on the active mode and applies them to the hardware
        void SetModeColors(const std::vector<Graphics::RGBA> &colors);

        /// Returns true if the active mode supports random color mode
        bool SupportsRandomColor() const;

        /// Returns true if the active mode is currently using random colors
        bool IsRandomColor() const;

        /// Enables or disables random color mode on the active mode (if supported)
        void SetRandomColor(bool random);

        /// Switches device to Direct / per-LED mode for custom software illumination
        bool SetDirectMode();

        // ---------------------------------------------------------
        // 2D LED Matrix & Strip Operations (Forwarded to Primary Matrix/Linear Zone)
        // ---------------------------------------------------------

        /// Returns the device's primary 2D matrix zone (or width x 1 linear strip zone), or nullopt if none
        std::optional<Zone> GetMatrixZone() const;

        /// Returns the 2D dimensions (width, height) of the device matrix/strip layout (width x 1 for strips), or {0, 0}
        Geometry::Size GetMatrixSize() const;

        /// Returns the width (columns / strip LED count) of the device matrix layout
        int GetWidth() const;

        /// Returns the height (rows; 1 for strips) of the device matrix layout
        int GetHeight() const;

        /// Gets LED pixel color at matrix coordinate (x, y), or transparent black (A=0) if unpopulated
        Graphics::RGBA GetPixel(int x, int y) const;
        Graphics::RGBA GetPixel(Geometry::Point p) const;

        /// Sets LED pixel color at matrix coordinate (x, y)
        void SetPixel(int x, int y, Graphics::RGBA color);
        void SetPixel(Geometry::Point p, Graphics::RGBA color);

        /// Draws a Bitmap onto the device matrix/strip zone (1:1 pixel mapping with offset)
        void Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset = {0, 0}, BlendMode blend = BlendMode::Overwrite);

        /// Draws image data onto the device matrix/strip zone
        void Draw(const Containers::Image &image, Geometry::Point offset = {0, 0}, BlendMode blend = BlendMode::Overwrite);

        /// Resamples and scales a Bitmap across the full dimensions of the device matrix/strip zone
        void DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp = Containers::InterpolationMethod::Linear, BlendMode blend = BlendMode::Overwrite);

        /// Resamples and scales image data across the full dimensions of the device matrix/strip zone
        void DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp = Containers::InterpolationMethod::Linear, BlendMode blend = BlendMode::Overwrite);

        /// Reads back the current state of the device matrix/strip into an Image container.
        /// Populated LEDs have A = 255; missing/unpopulated matrix entries are denoted with A = 0.
        Containers::Image ToImage() const;

    protected:
        struct Impl;
        std::shared_ptr<Impl> impl;
        const Gorgon::Animation::Governor *context = nullptr;
        explicit Device(std::shared_ptr<Impl> impl, const Gorgon::Animation::Governor *context = nullptr);

        friend struct Access;
        friend class Zone;
    };

    /**
     * @brief Specialized keyboard device with keycode mapping and embedded matrix/bitmap operations.
     */
    class KeyboardDevice : public Device {
    public:
        KeyboardDevice() = default;

        // ---------------------------------------------------------
        // Key-Specific Illumination
        // ---------------------------------------------------------

        /// Sets the color of a specific physical key using Gorgon's input keycode
        void SetKeyColor(Input::Key key, Graphics::RGBA color);

        /// Sets the color of a key by name (e.g. "Escape", "Space", "Enter")
        void SetKeyColor(const std::string &key_name, Graphics::RGBA color);

        /// Gets the base color of a specific key
        Graphics::RGBA GetKeyColor(Input::Key key) const;

        /// Sets the color of a key group (WASD, Arrows, Numpad, etc.)
        void SetKeyGroupColor(const KeyGroup &group, Graphics::RGBA color);

        // ---------------------------------------------------------
        // Key Queries
        // ---------------------------------------------------------

        /// Returns true if this keyboard has the specified key on its layout
        bool HasKey(Input::Key key) const;

        /// Returns the LED indices associated with the key (may be multiple for large keys like spacebar)
        std::vector<int> GetKeyLEDIndices(Input::Key key) const;

        /// Returns the 2D matrix coordinates (x, y) of the specified key if the keyboard has a matrix zone, or nullopt
        std::optional<Geometry::Point> GetKeyMatrixPosition(Input::Key key) const;

        /// Returns the Gorgon Input::Key mapped to the given LED index under the active OS keyboard layout, or 0 if unmapped
        Input::Key GetKeyForLED(int led_index) const;

        /// Returns the localized OS keycap label for the given LED index (e.g. "Ğ", "Ü", "Ş", "İ", "Ö", "Ç" on Turkish keyboards),
        /// falling back to the stripped OpenRGB LED name if not mapped to a localized OS key
        std::string GetKeyLabel(int led_index) const;

    private:
        explicit KeyboardDevice(std::shared_ptr<Impl> impl, const Gorgon::Animation::Governor *context = nullptr);
        friend struct Access;
    };

    /**
     * @brief Hardware LED progress indicator implementing Gorgon::ProgressDisplay.
     *
     * Composited non-destructively over the owning context's Base State. Removing or clearing
     * the progressbar automatically uncovers the current Base State underneath.
     */
    class Progressbar : public ProgressDisplay {
    public:
        /// Associates a progress ratio range [Start, End] with a specific active color
        struct LevelColor {
            Range<Float> range;
            Graphics::RGBA color;
        };

        /// Creates an unbound progress bar in the global context that does nothing until configured
        Progressbar();

        /// Creates a progress bar across the given key group on all connected keyboards
        explicit Progressbar(const KeyGroup &keys, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});

        /// Creates a progress bar across the given key group on a specific keyboard (inherits keyboard's governor context)
        Progressbar(const KeyGroup &keys, const KeyboardDevice &keyboard, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});

        /// Creates a progress bar across the given key group within a specific zone (inherits zone's governor context)
        Progressbar(const KeyGroup &keys, const Zone &zone, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});

        /// Creates a progress bar across all LEDs of a specific zone (e.g., LED strip; inherits zone's governor context)
        explicit Progressbar(const Zone &zone, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});

        /// Creates a progress bar across all LEDs of a specific device (e.g., LED strip; inherits device's governor context)
        explicit Progressbar(const Device &device, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});

        // ---------------------------------------------------------
        // Target & Context Configuration
        // ---------------------------------------------------------

        /// Returns the governor context this progressbar is attached to (nullptr for global layer)
        const Gorgon::Animation::Governor *GetGovernor() const;

        /// Sets the ordered list of keys to use for the progress bar
        void SetKeys(const KeyGroup &keys);
        const KeyGroup &GetKeys() const;

        /// Sets the target zone for the progress bar
        void SetZone(const Zone &zone);

        /// Sets the target device (e.g., LED strip) for the progress bar
        void SetDevice(const Device &device);

        /// Targets all connected LED strip devices in the owning context
        void TargetAllStrips();

        // ---------------------------------------------------------
        // Color & Level Configuration
        // ---------------------------------------------------------

        /// Sets the default foreground/active color (used when no level color matches)
        void SetColor(Graphics::RGBA color);
        Graphics::RGBA GetColor() const;

        /// Sets the background/unfilled color. If its alpha is 0 (default), unfilled LEDs keep
        /// the base state color underneath. Otherwise, the background is blended over the base state.
        void SetBackgroundColor(Graphics::RGBA bg);
        Graphics::RGBA GetBackgroundColor() const;

        /// Adds a level color rule for a specific progress range (e.g. Range<Float>(0.8f, 1.0f) -> Green)
        void AddLevelColor(Range<Float> range, Graphics::RGBA color);

        /// Replaces all level color rules
        void SetLevelColors(const std::vector<LevelColor> &levels);

        /// Removes all level color rules
        void ClearLevelColors();

        /// Returns the configured level color rules
        const std::vector<LevelColor> &GetLevelColors() const;

        // ---------------------------------------------------------
        // Display Mode (Filling vs. Single Key Indicator)
        // ---------------------------------------------------------

        /// If true (default), fills LEDs from the start up to the current progress ratio.
        /// If false, illuminates only the single interpolated key/LED at the current progress position.
        void SetFillMode(bool fill);
        bool IsFillMode() const;

        /// Convenience toggle inverse of SetFillMode: true illuminates only a single sliding key/LED
        void SetSingleKeyMode(bool single) { SetFillMode(!single); }
        bool IsSingleKeyMode() const { return !IsFillMode(); }

        // ---------------------------------------------------------
        // ProgressDisplay Interface
        // ---------------------------------------------------------

        /// Updates the progress ratio in [0.0, 1.0] and refreshes target LEDs
        void SetProgressRatio(Float value) override;

        /// Returns the current progress ratio in [0.0, 1.0]
        Float GetProgressRatio() const override;

        /// Stops displaying progress and uncovers the base state colors underneath
        void Clear();

    private:
        struct Impl;
        std::shared_ptr<Impl> impl;
        friend struct Access;
        friend class Context;
    };

    /**
     * @brief Base class for software-driven OpenRGB lighting animations and effects.
     *
     * Integrates directly with `Gorgon::Animation`:
     * - Every `OpenRGB::Animation` owns (or attaches to) a `Gorgon::Animation::Controller` bound to
     *   the `Animation::Governor` (or `Scene`) active at construction time (or explicitly specified).
     * - When the owning `Scene` / `Governor` is inactive, the animation automatically pauses and
     *   its layer is hidden; when the `Scene` is activated again, it automatically resumes.
     * - Exposes `FinishedEvent` from its `Animation::Controller` so effects can be chained
     *   (e.g., instant flash red -> fade out to black).
     * - Composited non-destructively over the layer's Base State (`base_leds`). When stopped,
     *   removed, or finished (if `KeepWhenFinished` is false), the keyboard returns to its Base State.
     */
    class Animation : public Gorgon::Animation::Base {
    public:
        /// Creates an animation bound to the currently active `Animation::Governor` (`Governor::Active()`)
        explicit Animation(unsigned duration_ms = 1000, BlendMode blend = BlendMode::AlphaBlend);

        /// Creates an animation bound to a specific `Animation::Governor` (or `Scene`)
        Animation(Gorgon::Animation::Governor &governor, unsigned duration_ms = 1000, BlendMode blend = BlendMode::AlphaBlend);

        /// Creates an animation bound to a specific `OpenRGB::Context` (e.g. `scene.OpenRGB`)
        Animation(const Context &context, unsigned duration_ms = 1000, BlendMode blend = BlendMode::AlphaBlend);

        /// Copy constructor (shares the underlying effect state)
        Animation(const Animation &other);
        Animation &operator=(const Animation &other);

        virtual ~Animation();

        // ---------------------------------------------------------
        // Target Selection
        // ---------------------------------------------------------

        /// Targets all connected devices in the owning context (default)
        void TargetAllDevices();

        /// Targets all connected keyboards in the owning context (optionally restricted to a KeyGroup)
        void TargetAllKeyboards();
        void TargetKeyGroup(const KeyGroup &group);
        void TargetKey(Input::Key key);

        /// Targets all connected LED strip devices in the owning context
        void TargetAllStrips();

        /// Targets a specific device (and switches governor context if the device handle has one)
        void TargetDevice(const Device &device);

        /// Targets a specific keyboard (optionally restricted to a KeyGroup or single Key)
        void TargetKeyboard(const KeyboardDevice &keyboard);
        void TargetKeyboardKeys(const KeyboardDevice &keyboard, const KeyGroup &group);
        void TargetKeyboardKey(const KeyboardDevice &keyboard, Input::Key key);

        /// Targets a specific zone
        void TargetZone(const Zone &zone);

        // ---------------------------------------------------------
        // Playback & Controller Integration
        // ---------------------------------------------------------

        /// Starts playing the animation once from the beginning (or resumes if paused)
        void Play();

        /// Starts playing the animation in continuous looping mode
        void Loop();

        /// Pauses the animation at its current frame
        void Pause();

        /// Resumes a paused animation
        void Resume();

        /// Resets progress to 0 and restarts state
        void Reset();

        /// Stops the animation and removes its overlay from the LEDs, returning them to the Base State
        void Stop();

        /// Returns true if currently playing
        bool IsPlaying() const;

        /// Returns true if paused
        bool IsPaused() const;

        /// Returns true if a non-looping animation has reached the end
        bool IsFinished() const;

        /// Sets playback speed multiplier (default 1.0f; negative runs in reverse)
        void SetSpeed(float speed);
        float GetSpeed() const;

        /// Sets total duration in milliseconds
        void SetDuration(unsigned duration_ms);

        /// Returns total duration in milliseconds
        int GetDuration() const override;

        /// Sets how this animation blends over the Base State (and earlier animations)
        void SetBlendMode(BlendMode blend);
        BlendMode GetBlendMode() const;

        /// If false (default), finishing a one-shot animation automatically removes its overlay
        /// so LEDs return to the Base State. If true, the final frame remains visible until Stop()/Reset().
        void SetKeepWhenFinished(bool keep);
        bool GetKeepWhenFinished() const;

        /// Returns the underlying `Gorgon::Animation::Controller`
        Gorgon::Animation::Controller &GetAnimationController();
        const Gorgon::Animation::Controller &GetAnimationController() const;

        /// Returns the governor this animation is bound to
        const Gorgon::Animation::Governor *GetGovernor() const;

        /// Moves this animation to another governor (updates both the `Animation::Controller` and OpenRGB context)
        void SetGovernor(Gorgon::Animation::Governor &governor);

        /// Fired on the main thread when a one-shot (non-looping) animation finishes playing.
        /// Can be used to chain animations sequentially.
        Gorgon::Event<Gorgon::Animation::Controller> &FinishedEvent;

        // ---------------------------------------------------------
        // Gorgon::Animation::Base Interface
        // ---------------------------------------------------------

        bool Progress(unsigned &leftover) override;
        void DeleteAnimation() const override { delete this; }

    protected:
        struct Impl;
        std::shared_ptr<Impl> impl;
        explicit Animation(std::shared_ptr<Impl> impl);

        friend struct Access;
        friend class Context;
    };

    /**
     * @brief Smoothly interpolates (fades) target LEDs across two or more colors over `duration_ms`.
     *
     * Ideal for death animations (e.g., instant full red fading slowly to black or transparent),
     * damage flashes, or multi-stop color transitions.
     */
    class FadeEffect : public Animation {
    public:
        FadeEffect(Graphics::RGBA start_color, Graphics::RGBA end_color, unsigned duration_ms = 1000, BlendMode blend = BlendMode::AlphaBlend);
        FadeEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA start_color, Graphics::RGBA end_color, unsigned duration_ms = 1000, BlendMode blend = BlendMode::AlphaBlend);
        FadeEffect(const Context &context, Graphics::RGBA start_color, Graphics::RGBA end_color, unsigned duration_ms = 1000, BlendMode blend = BlendMode::AlphaBlend);

        /// Sets two-color start and end stops
        void SetColors(Graphics::RGBA start_color, Graphics::RGBA end_color);

        /// Sets an arbitrary list of equally-spaced color stops (at least 2)
        void SetColorStops(const std::vector<Graphics::RGBA> &stops);
    };

    /**
     * @brief Smoothly pulses/breathes target LEDs between two colors (or a color and transparent) using a sine wave.
     */
    class PulseEffect : public Animation {
    public:
        PulseEffect(Graphics::RGBA peak_color, unsigned period_ms = 1200, Graphics::RGBA trough_color = {0x00000000}, BlendMode blend = BlendMode::AlphaBlend);
        PulseEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA peak_color, unsigned period_ms = 1200, Graphics::RGBA trough_color = {0x00000000}, BlendMode blend = BlendMode::AlphaBlend);
        PulseEffect(const Context &context, Graphics::RGBA peak_color, unsigned period_ms = 1200, Graphics::RGBA trough_color = {0x00000000}, BlendMode blend = BlendMode::AlphaBlend);

        void SetColors(Graphics::RGBA peak_color, Graphics::RGBA trough_color = {0x00000000});
    };

    /**
     * @brief Blinks target LEDs between an on-color and off-color with configurable duty cycle.
     */
    class BlinkEffect : public Animation {
    public:
        BlinkEffect(Graphics::RGBA on_color, unsigned period_ms = 500, float duty_cycle = 0.5f, Graphics::RGBA off_color = {0x00000000}, BlendMode blend = BlendMode::AlphaBlend);
        BlinkEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA on_color, unsigned period_ms = 500, float duty_cycle = 0.5f, Graphics::RGBA off_color = {0x00000000}, BlendMode blend = BlendMode::AlphaBlend);
        BlinkEffect(const Context &context, Graphics::RGBA on_color, unsigned period_ms = 500, float duty_cycle = 0.5f, Graphics::RGBA off_color = {0x00000000}, BlendMode blend = BlendMode::AlphaBlend);

        void SetColors(Graphics::RGBA on_color, Graphics::RGBA off_color = {0x00000000});
        void SetDutyCycle(float duty_cycle);
    };

    /**
     * @brief Radial shockwave / explosion effect expanding outward from a 2D matrix coordinate,
     *        normalized center, or specific `Input::Key` (e.g., when taking damage or hitting a mole).
     */
    class ExplosionEffect : public Animation {
    public:
        /// Creates a radial explosion centered on the matrix (normalized center {0.5f, 0.5f})
        ExplosionEffect(Graphics::RGBA color, unsigned duration_ms = 700, float ring_thickness = 3.5f, BlendMode blend = BlendMode::AlphaBlend);
        ExplosionEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA color, unsigned duration_ms = 700, float ring_thickness = 3.5f, BlendMode blend = BlendMode::AlphaBlend);
        ExplosionEffect(const Context &context, Graphics::RGBA color, unsigned duration_ms = 700, float ring_thickness = 3.5f, BlendMode blend = BlendMode::AlphaBlend);

        /// Sets the explosion origin to a specific physical key on the keyboard matrix
        void SetOriginKey(Input::Key key);

        /// Sets the explosion origin to an explicit matrix pixel coordinate (x, y)
        void SetOriginPoint(Geometry::Point matrix_coord);

        /// Sets the explosion origin in normalized [0..1, 0..1] matrix coordinates (default {0.5f, 0.5f})
        void SetOriginNormalized(Geometry::Pointf norm_coord);

        /// Sets primary shockwave color and optional inner core/trail color
        void SetColor(Graphics::RGBA color);

        /// Sets the radial thickness of the expanding wave in matrix cells (or 0 for a solid filled circle)
        void SetRingThickness(float thickness);

        /// Sets the maximum radius in matrix cells (0 = auto-compute to cover the full matrix)
        void SetMaxRadius(float max_radius);
    };

    /**
     * @brief Linear wave / swipe effect sweeping a band of color across a keyboard matrix or LED zone
     *        (e.g., when picking up a buff or transitioning states).
     */
    class SwipeEffect : public Animation {
    public:
        SwipeEffect(Graphics::RGBA color, SwipeDirection direction = SwipeDirection::LeftToRight, unsigned duration_ms = 600, float band_width = 0.25f, BlendMode blend = BlendMode::AlphaBlend);
        SwipeEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA color, SwipeDirection direction = SwipeDirection::LeftToRight, unsigned duration_ms = 600, float band_width = 0.25f, BlendMode blend = BlendMode::AlphaBlend);
        SwipeEffect(const Context &context, Graphics::RGBA color, SwipeDirection direction = SwipeDirection::LeftToRight, unsigned duration_ms = 600, float band_width = 0.25f, BlendMode blend = BlendMode::AlphaBlend);

        void SetColor(Graphics::RGBA color);
        void SetDirection(SwipeDirection direction);

        /// Sets the width of the sweeping band as a fraction of the total matrix/zone length (e.g. 0.25f)
        void SetBandWidth(float width_ratio);
    };

    /**
     * @brief Software-driven per-key reactive lighting effect in Direct Mode.
     *
     * Can be active simultaneously with static key highlights (Base State), progressbars, and
     * other animations! Whenever a key is pressed (either triggered via `TriggerKey(key)` or
     * automatically hooked to a `UI::Window` / `SceneManager`), that key (or a radial ripple
     * around it) lights up with `color` and smoothly fades back to the Base State over `fade_ms`.
     */
    class ReactiveEffect : public Animation {
    public:
        explicit ReactiveEffect(Graphics::RGBA color = {0xffffffff}, unsigned fade_ms = 500, BlendMode blend = BlendMode::AlphaBlend);
        ReactiveEffect(Gorgon::Animation::Governor &governor, Graphics::RGBA color = {0xffffffff}, unsigned fade_ms = 500, BlendMode blend = BlendMode::AlphaBlend);
        ReactiveEffect(const Context &context, Graphics::RGBA color = {0xffffffff}, unsigned fade_ms = 500, BlendMode blend = BlendMode::AlphaBlend);

        /// Manually triggers a key press flash on the given key
        void TriggerKey(Input::Key key);

        /// Automatically listens to `window.KeyPreviewEvent` and triggers whenever a key is pressed
        /// while this animation's governor is active
        void AttachToWindow(Gorgon::Window &window);

        /// Stops listening to the attached window's KeyPreviewEvent
        void DetachFromWindow();

        /// Sets the reactive flash color (or enables rainbow/random color per key press)
        void SetColor(Graphics::RGBA color);
        void SetRandomColor(bool random);

        /// Sets how long each key takes to fade back to the Base State in milliseconds
        void SetFadeDuration(unsigned fade_ms);

        /// If > 0, pressing a key also emits a small radial ripple to neighboring matrix keys
        void SetRippleRadius(float radius_cells);
    };

    // -------------------------------------------------------------
    // Global Subsystem Lifecycle & Configuration
    // -------------------------------------------------------------

    /// Explicitly initializes the OpenRGB subsystem and connects to the server.
    /// Note: Initialization is lazy and automatic on the first call to any lighting function.
    bool Initialize(const std::string &host = "127.0.0.1", uint16_t port = 6742);

    /// Returns true if connected to the OpenRGB server and initialized
    bool IsAvailable();

    /// Reverts all hardware to its initial state (if restore is true), disconnects, and terminates background worker.
    /// Registered to run automatically upon engine shutdown.
    void Shutdown(bool restore = true);

    /// Reverts all connected hardware to their original captured state without disconnecting
    void RestoreAll();

    /// Temporarily restores all connected devices to their original OEM hardware lighting
    /// while keeping all software layers, animations, progressbars, and action keys intact in memory.
    /// Hardware updates are paused until Resume() is called.
    void Suspend();

    /// Resumes software RGB lighting after Suspend(), re-engaging hardware direct mode
    /// and pushing all software layers and active animations back to the physical devices.
    void Resume();

    /// Returns true if hardware updates are currently suspended
    bool IsSuspended();

    /// Alias for Suspend() providing a temporary force restore of OEM hardware state
    inline void ForceRestore() { Suspend(); }

    /// Captures the current software layer state of all devices
    StateSnapshot CaptureState();

    /// Restores previously captured software layer state across all devices
    void RestoreState(const StateSnapshot &state);

    /// Immediately flushes all dirty / pending LED changes over the network
    void Flush();

    /// Sets the background worker update rate (default: 30 FPS, range: 10-120)
    void SetTargetFPS(int fps);

    /// Returns the current target update rate
    int GetTargetFPS();

    /// Returns the global OpenRGB context (operates on the Global Layer regardless of active Scene/Governor)
    inline constexpr Context GlobalContext() noexcept { return Context(nullptr); }

    // -------------------------------------------------------------
    // Device Enumeration (Global Layer)
    // -------------------------------------------------------------

    /// Refreshes device list from the server, rescanning hardware buses
    void Refresh();

    /// Returns all connected OpenRGB devices bound to the Global Layer (set only_connected to false to include detached devices)
    std::vector<Device> GetDevices(bool only_connected = true);

    /// Returns only keyboard devices bound to the Global Layer (set only_connected to false to include detached keyboards)
    std::vector<KeyboardDevice> GetKeyboards(bool only_connected = true);

    /// Returns only LED strip devices bound to the Global Layer (set only_connected to false to include detached strips)
    std::vector<Device> GetStrips(bool only_connected = true);

    /// Finds a device by name (case-insensitive match) bound to the Global Layer, or nullopt
    std::optional<Device> FindDevice(const std::string &name);

    /// Returns the primary keyboard (first available) bound to the Global Layer, or nullopt
    std::optional<KeyboardDevice> GetPrimaryKeyboard();

    /// Returns the primary LED strip (first available) bound to the Global Layer, or nullopt
    std::optional<Device> GetPrimaryStrip();

    // -------------------------------------------------------------
    // Global Convenience Functions (Always Modify Global Layer)
    // -------------------------------------------------------------

    /// Sets key color across all connected keyboards in the Global Layer
    void SetKeyColor(Input::Key key, Graphics::RGBA color);

    /// Sets key group color across all connected keyboards in the Global Layer
    void SetKeyGroupColor(const KeyGroup &group, Graphics::RGBA color);

    /// Draws a Bitmap onto all connected keyboards and matrix/strip zones in the Global Layer
    void Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset = {0, 0}, BlendMode blend = BlendMode::Overwrite);

    /// Draws image data onto all connected keyboards and matrix/strip zones in the Global Layer
    void Draw(const Containers::Image &image, Geometry::Point offset = {0, 0}, BlendMode blend = BlendMode::Overwrite);

    /// Scales and draws a Bitmap across all connected keyboards and matrix/strip zones in the Global Layer
    void DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp = Containers::InterpolationMethod::Linear, BlendMode blend = BlendMode::Overwrite);

    /// Scales and draws image data across all connected keyboards and matrix/strip zones in the Global Layer
    void DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp = Containers::InterpolationMethod::Linear, BlendMode blend = BlendMode::Overwrite);

    /// Sets solid color across ALL connected devices in the Global Layer
    void SetAllColor(Graphics::RGBA color);

    /// Turns off all LEDs across all devices in the Global Layer
    void ClearAll();

    /// Creates a progressbar in the Global Layer
    Progressbar CreateProgressbar();
    Progressbar CreateProgressbar(const KeyGroup &keys, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});
    Progressbar CreateProgressbar(const KeyGroup &keys, const KeyboardDevice &keyboard, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});
    Progressbar CreateProgressbar(const KeyGroup &keys, const Zone &zone, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});
    Progressbar CreateProgressbar(const Zone &zone, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});
    Progressbar CreateProgressbar(const Device &device, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});

    /// Moves an existing progressbar to the Global Layer
    void AttachProgressbar(Progressbar &progressbar);

    /// Moves an existing OpenRGB animation/effect to the Global Layer
    void AttachAnimation(Animation &animation);

    // -------------------------------------------------------------
    // Action Keys & Shortcut Highlighting
    // -------------------------------------------------------------

    using Input::ModifierHighlightMode;
    using Input::Shortcut;

    namespace internal {
        struct ShortcutBinding {
            Input::Key key = 0;
            Input::Keyboard::Modifier modifier = Input::Keyboard::Modifier::None;
            Graphics::RGBA color = {0,0,0,0};
        };

        struct ShortcutOverlay {
            const Gorgon::Animation::Governor *context = nullptr;
            ModifierHighlightMode mode = ModifierHighlightMode::ActiveModifiers;
            Graphics::RGBA modifier_key_color = Graphics::RGBA(60, 160, 255, 255);
            bool active = true;
            std::vector<ShortcutBinding> bindings;
        };

        std::shared_ptr<ShortcutOverlay> registerShortcutOverlay(const Gorgon::Animation::Governor *context);
        void updateShortcutOverlay(const std::shared_ptr<ShortcutOverlay> &overlay);
        void unregisterShortcutOverlay(const std::shared_ptr<ShortcutOverlay> &overlay);
    }

    /// Sets modifier-aware action key highlights across all connected keyboards in the Global Layer
    void SetActionKeys(const std::vector<internal::ShortcutBinding> &bindings, ModifierHighlightMode mode = ModifierHighlightMode::ActiveModifiers);

    /// Clears modifier-aware action key highlights in the Global Layer
    void ClearActionKeys();

    // -------------------------------------------------------------
    // Events & Logging
    // -------------------------------------------------------------

    /// Fired when devices are connected, enumerated, or updated
    extern Event<void> DeviceConnectedEvent;

    /// Fired when a communication or protocol error occurs (never crashes)
    extern Event<void, const std::string &> ErrorEvent;

    /// Used for debugging.
    extern Utils::Logger Logger;

} // namespace Gorgon::Devices::OpenRGB
