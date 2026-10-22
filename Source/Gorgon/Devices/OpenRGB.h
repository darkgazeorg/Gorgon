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

#include "../Graphics/Color.h"
#include "../Geometry/Point.h"
#include "../Geometry/Size.h"
#include "../Containers/Image.h"
#include "../Event.h"
#include "../Input.h"
#include "../Input/Keyboard.h"
#include "../Progress.h"
#include "../Utils/Logging.h"

namespace Gorgon :: Graphics {
    class Bitmap;
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

    /// Blending strategy when drawing a Bitmap or Image onto an LED matrix
    enum class BlendMode {
        Overwrite,       ///< Pixel color directly replaces existing LED color
        AlphaBlend       ///< Blends pixel over current LED color using the pixel's alpha channel
    };

    /// Metadata for a single physical LED
    struct LED {
        int index = 0;
        std::string name;
        Graphics::RGBA color = {0xff000000};
    };

    class Device;

    /// Internal helper granting the implementation file access to private state.
    struct Access;

    /**
     * @brief Represents an addressable lighting zone on a device (e.g. "Keyboard", "Underglow", "Matrix").
     */
    class Zone {
    public:
        Zone();
        Zone(int index, const std::string &name, ZoneType type, int led_count, int led_offset, Device &parent);

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

        /// Returns the parent device this zone belongs to
        Device &GetDevice() const;

        // ---------------------------------------------------------
        // Color Controls
        // ---------------------------------------------------------

        /// Returns the current color of a zone-local LED
        Graphics::RGBA GetLEDColor(int local_idx) const;

        /// Sets the color of a zone-local LED
        void SetLEDColor(int local_idx, Graphics::RGBA color);

        /// Sets all LEDs in this zone to a solid color
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

        /// Reads back the current state of this matrix into an Image container.
        /// Populated LEDs have A = 255; missing/unpopulated matrix entries are denoted with A = 0.
        Containers::Image ToImage() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> impl;
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

        // ---------------------------------------------------------
        // LEDs & Direct Colors (Across All Zones)
        // ---------------------------------------------------------

        /// Total number of LEDs on this device across all zones
        int GetLEDCount() const;

        /// Returns metadata for all LEDs on this device
        const std::vector<LED> &GetLEDs() const;

        /// Finds an LED by name (case-insensitive search), returns index or -1
        int FindLED(const std::string &name) const;

        /// Gets the current color of an LED by global device index
        Graphics::RGBA GetLEDColor(int led_idx) const;

        /// Gets the current color of an LED by name (or transparent black if not found)
        Graphics::RGBA GetLEDColor(const std::string &name) const;

        /// Sets the color of an LED by global device index
        void SetLEDColor(int led_idx, Graphics::RGBA color);

        /// Sets the color of an LED by name
        void SetLEDColor(const std::string &name, Graphics::RGBA color);

        /// Sets all LEDs across the entire device to a solid color
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

        /// Finds a zone by name, or nullptr
        Zone *FindZone(const std::string &name);

        /// Returns all zones belonging to this device
        std::vector<Zone> GetZones() const;

        // ---------------------------------------------------------
        // Hardware Modes
        // ---------------------------------------------------------

        int GetModeCount() const;
        Mode GetActiveMode() const;
        std::string GetActiveModeName() const;
        void SetMode(Mode mode_index);
        void SetMode(const std::string &mode_name);

        /// Switches device to Direct / per-LED mode for custom software illumination
        bool SetDirectMode();

    protected:
        struct Impl;
        std::shared_ptr<Impl> impl;
        explicit Device(std::shared_ptr<Impl> impl);

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

        /// Gets the color of a specific key
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

        // ---------------------------------------------------------
        // 2D LED Matrix & Bitmap Operations (Forwarded to Keyboard Zone)
        // ---------------------------------------------------------

        /// Returns the keyboard's 2D matrix zone, or nullptr if the keyboard has no matrix layout
        Zone *GetMatrixZone();
        const Zone *GetMatrixZone() const;

        /// Returns the 2D dimensions (width, height) of the keyboard matrix layout, or {0, 0}
        Geometry::Size GetMatrixSize() const;

        /// Returns the width (columns) of the keyboard matrix layout
        int GetWidth() const;

        /// Returns the height (rows) of the keyboard matrix layout
        int GetHeight() const;

        /// Gets LED pixel color at keyboard matrix coordinate (x, y), or transparent black (A=0) if unpopulated
        Graphics::RGBA GetPixel(int x, int y) const;
        Graphics::RGBA GetPixel(Geometry::Point p) const;

        /// Sets LED pixel color at keyboard matrix coordinate (x, y)
        void SetPixel(int x, int y, Graphics::RGBA color);
        void SetPixel(Geometry::Point p, Graphics::RGBA color);

        /// Draws a Bitmap onto the keyboard matrix zone (1:1 pixel mapping with offset)
        void Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset = {0, 0}, BlendMode blend = BlendMode::Overwrite);

        /// Draws image data onto the keyboard matrix zone
        void Draw(const Containers::Image &image, Geometry::Point offset = {0, 0}, BlendMode blend = BlendMode::Overwrite);

        /// Resamples and scales a Bitmap across the full dimensions of the keyboard matrix zone
        void DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp = Containers::InterpolationMethod::Linear, BlendMode blend = BlendMode::Overwrite);

        /// Resamples and scales image data across the full dimensions of the keyboard matrix zone
        void DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp = Containers::InterpolationMethod::Linear, BlendMode blend = BlendMode::Overwrite);

        /// Reads back the current state of the keyboard matrix into an Image container.
        /// Populated LEDs have A = 255; missing/unpopulated matrix entries are denoted with A = 0.
        Containers::Image ToImage() const;

    private:
        explicit KeyboardDevice(std::shared_ptr<Impl> impl);
        friend struct Access;
    };

    /**
     * @brief Hardware LED progress indicator implementing Gorgon::ProgressDisplay.
     *
     * Can drive an ordered list of keys (across all keyboards, a specific keyboard, or a zone)
     * or all LEDs of a linear/matrix Zone. Supports both continuous filling and single-key
     * sliding indicator modes, as well as threshold/range-based level colors (e.g., green > 80%,
     * red < 20%, yellow otherwise).
     */
    class Progressbar : public ProgressDisplay {
    public:
        /// Associates a progress ratio range [Start, End] with a specific active color
        struct LevelColor {
            Range<Float> range;
            Graphics::RGBA color;
        };

        /// Creates an unbound progress bar that does nothing until configured
        Progressbar();

        /// Creates a progress bar across the given key group on all connected keyboards
        explicit Progressbar(const KeyGroup &keys, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});

        /// Creates a progress bar across the given key group on a specific keyboard
        Progressbar(const KeyGroup &keys, KeyboardDevice &keyboard, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});

        /// Creates a progress bar across the given key group within a specific zone
        Progressbar(const KeyGroup &keys, Zone &zone, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});

        /// Creates a progress bar across all LEDs of a specific zone (e.g., LED strip)
        explicit Progressbar(Zone &zone, Graphics::RGBA color = {0xff00ff00}, Graphics::RGBA bg = {0x00000000});

        // ---------------------------------------------------------
        // Target Configuration
        // ---------------------------------------------------------

        /// Sets the ordered list of keys to use for the progress bar
        void SetKeys(const KeyGroup &keys);
        const KeyGroup &GetKeys() const;

        /// Sets the target zone for the progress bar
        void SetZone(Zone &zone);

        // ---------------------------------------------------------
        // Color & Level Configuration
        // ---------------------------------------------------------

        /// Sets the default foreground/active color (used when no level color matches)
        void SetColor(Graphics::RGBA color);
        Graphics::RGBA GetColor() const;

        /// Sets the background/unfilled color. If its alpha is 0 (default), unfilled LEDs keep
        /// the color they had before the progress bar touched them. Otherwise, the background
        /// is blended over that color using the alpha value.
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

        /// Stops displaying progress and restores the LEDs to the colors they had
        /// before the progress bar first modified them.
        void Clear();

    private:
        struct Impl;
        std::shared_ptr<Impl> impl;
        friend struct Access;
    };

    // -------------------------------------------------------------
    // Global Subsystem Lifecycle & Configuration
    // -------------------------------------------------------------

    /// Explicitly initializes the OpenRGB subsystem and connects to the server.
    /// Note: Initialization is lazy and automatic on the first call to any lighting function.
    bool Initialize(const std::string &host = "127.0.0.1", uint16_t port = 6742);

    /// Returns true if connected to the OpenRGB server and initialized
    bool IsAvailable();

    /// Reverts all hardware to its initial state, disconnects, and terminates background worker.
    /// Registered to run automatically upon engine shutdown.
    void Shutdown();

    /// Reverts all connected hardware to their original captured state without disconnecting
    void RestoreAll();

    /// Immediately flushes all dirty / pending LED changes over the network
    void Flush();

    /// Sets the background worker update rate (default: 30 FPS, range: 10-120)
    void SetTargetFPS(int fps);

    /// Returns the current target update rate
    int GetTargetFPS();

    // -------------------------------------------------------------
    // Device Enumeration
    // -------------------------------------------------------------

    /// Returns all connected OpenRGB devices
    std::vector<Device> GetDevices();

    /// Returns only keyboard devices
    std::vector<KeyboardDevice> GetKeyboards();

    /// Finds a device by name (case-insensitive match), or nullptr
    Device *FindDevice(const std::string &name);

    /// Returns the primary keyboard (first available), or nullptr
    KeyboardDevice *GetPrimaryKeyboard();

    // -------------------------------------------------------------
    // Global Convenience Functions (Zero Boilerplate)
    // -------------------------------------------------------------

    /// Sets key color across all connected keyboards
    void SetKeyColor(Input::Key key, Graphics::RGBA color);

    /// Sets key group color across all connected keyboards
    void SetKeyGroupColor(const KeyGroup &group, Graphics::RGBA color);

    /// Draws a Bitmap onto all connected keyboards and matrix zones
    void Draw(const Graphics::Bitmap &bitmap, Geometry::Point offset = {0, 0}, BlendMode blend = BlendMode::Overwrite);

    /// Draws image data onto all connected keyboards and matrix zones
    void Draw(const Containers::Image &image, Geometry::Point offset = {0, 0}, BlendMode blend = BlendMode::Overwrite);

    /// Scales and draws a Bitmap across all connected keyboards and matrix zones
    void DrawScaled(const Graphics::Bitmap &bitmap, Containers::InterpolationMethod interp = Containers::InterpolationMethod::Linear, BlendMode blend = BlendMode::Overwrite);

    /// Scales and draws image data across all connected keyboards and matrix zones
    void DrawScaled(const Containers::Image &image, Containers::InterpolationMethod interp = Containers::InterpolationMethod::Linear, BlendMode blend = BlendMode::Overwrite);

    /// Sets solid color across ALL connected devices
    void SetAllColor(Graphics::RGBA color);

    /// Turns off all LEDs across all devices
    void ClearAll();

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
