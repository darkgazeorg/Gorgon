/**
 * @file OpenRGBClient.h
 * @brief Standalone, zero-dependency OpenRGB C++17 client library.
 *
 * Implements the OpenRGB binary network protocol (versions 0 through 5).
 * Designed to be completely self-contained with no external library dependencies
 * outside the C++17 standard library and standard OS sockets.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <utility>

#ifdef OPENRGB_CLIENT_STANDALONE
namespace openrgb {
#else
namespace Gorgon :: Devices :: OpenRGB :: LowLevel {
#endif

    // Protocol Constants
    inline constexpr uint32_t OPENRGB_SDK_PROTOCOL_VERSION = 5;
    inline constexpr uint16_t OPENRGB_DEFAULT_PORT = 6742;

    // Packet IDs
    inline constexpr uint32_t NET_PACKET_ID_REQUEST_CONTROLLER_COUNT       = 0;
    inline constexpr uint32_t NET_PACKET_ID_REQUEST_CONTROLLER_DATA        = 1;
    inline constexpr uint32_t NET_PACKET_ID_REQUEST_PROTOCOL_VERSION       = 40;
    inline constexpr uint32_t NET_PACKET_ID_SET_CLIENT_NAME                = 50;
    inline constexpr uint32_t NET_PACKET_ID_DEVICE_LIST_UPDATED            = 100;
    inline constexpr uint32_t NET_PACKET_ID_REQUEST_RESCAN_DEVICES         = 140;
    inline constexpr uint32_t NET_PACKET_ID_RGBCONTROLLER_UPDATELEDS       = 1050;
    inline constexpr uint32_t NET_PACKET_ID_RGBCONTROLLER_UPDATEZONELEDS   = 1051;
    inline constexpr uint32_t NET_PACKET_ID_RGBCONTROLLER_UPDATESINGLELED  = 1052;
    inline constexpr uint32_t NET_PACKET_ID_RGBCONTROLLER_SETCUSTOMMODE    = 1100;
    inline constexpr uint32_t NET_PACKET_ID_RGBCONTROLLER_UPDATEMODE       = 1101;
    inline constexpr uint32_t NET_PACKET_ID_RGBCONTROLLER_SAVEMODE         = 1102;

    /**
     * @brief 32-bit Color representation identical to OpenRGB wire format (0x00BBGGRR).
     *
     * In little-endian memory layout, bytes are ordered [R, G, B, A].
     * 100% binary-compatible with Gorgon::Graphics::RGBA.
     */
    struct Color {
        uint32_t value = 0;

        constexpr Color() = default;
        constexpr Color(uint32_t val) : value(val) {}
        constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
            : value(static_cast<uint32_t>(r) |
                   (static_cast<uint32_t>(g) << 8) |
                   (static_cast<uint32_t>(b) << 16) |
                   (static_cast<uint32_t>(a) << 24)) {}

        static constexpr Color FromRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
            return Color(r, g, b, a);
        }

        constexpr uint8_t R() const { return static_cast<uint8_t>(value & 0xFF); }
        constexpr uint8_t G() const { return static_cast<uint8_t>((value >> 8) & 0xFF); }
        constexpr uint8_t B() const { return static_cast<uint8_t>((value >> 16) & 0xFF); }
        constexpr uint8_t A() const { return static_cast<uint8_t>((value >> 24) & 0xFF); }

        constexpr operator uint32_t() const { return value; }
        constexpr bool operator==(const Color &other) const { return value == other.value; }
        constexpr bool operator!=(const Color &other) const { return value != other.value; }
    };

    /**
     * @brief OpenRGB device categories.
     */
    enum class DeviceType : int32_t {
        Motherboard  = 0,
        DRAM         = 1,
        GPU          = 2,
        Cooler       = 3,
        LedStrip     = 4,
        Keyboard     = 5,
        Mouse        = 6,
        MouseMat     = 7,
        Headset      = 8,
        HeadsetStand = 9,
        Gamepad      = 10,
        Light        = 11,
        Speaker      = 12,
        Virtual      = 13,
        Unknown      = 14
    };

    /**
     * @brief OpenRGB zone layout geometries.
     */
    enum class ZoneType : int32_t {
        Single = 0,
        Linear = 1,
        Matrix = 2
    };

    /**
     * @brief Mode capability bitflags.
     */
    namespace ModeFlags {
        inline constexpr uint32_t HasSpeed             = (1 << 0);
        inline constexpr uint32_t HasDirectionLR       = (1 << 1);
        inline constexpr uint32_t HasDirectionUD       = (1 << 2);
        inline constexpr uint32_t HasDirectionHV       = (1 << 3);
        inline constexpr uint32_t HasBrightness        = (1 << 4);
        inline constexpr uint32_t HasPerLedColor       = (1 << 5);
        inline constexpr uint32_t HasModeSpecificColor = (1 << 6);
        inline constexpr uint32_t HasRandomColor       = (1 << 7);
    }

    /**
     * @brief Mode animation direction.
     */
    enum class ModeDirection : uint32_t {
        Left       = 0,
        Right      = 1,
        Up         = 2,
        Down       = 3,
        Horizontal = 4,
        Vertical   = 5
    };

    /**
     * @brief Mode color assignment mode.
     */
    enum class ModeColorMode : uint32_t {
        None         = 0,
        PerLed       = 1,
        ModeSpecific = 2,
        Random       = 3
    };

    /**
     * @brief 2D LED matrix map for keyboards and grid zones.
     */
    struct MatrixMap {
        uint32_t height = 0;
        uint32_t width = 0;
        std::vector<uint32_t> matrix; // Size: height * width. Unpopulated entries are 0xFFFFFFFF.

        uint32_t Get(uint32_t row, uint32_t col) const {
            if (row < height && col < width) {
                size_t idx = static_cast<size_t>(row) * width + col;
                if (idx < matrix.size()) return matrix[idx];
            }
            return 0xFFFFFFFF;
        }
    };

    /**
     * @brief Zone segment metadata (Protocol v4+).
     */
    struct Segment {
        std::string name;
        int32_t     type = 0;
        uint32_t    start_idx = 0;
        uint32_t    leds_count = 0;
    };

    /**
     * @brief Controller zone description.
     */
    struct Zone {
        std::string          name;
        ZoneType             type = ZoneType::Single;
        uint32_t             leds_min = 0;
        uint32_t             leds_max = 0;
        uint32_t             leds_count = 0;
        MatrixMap            matrix_map;
        std::vector<Segment> segments;
    };

    /**
     * @brief Hardware lighting mode settings.
     */
    struct Mode {
        std::string        name;
        int32_t            value = 0;
        uint32_t           flags = 0;
        uint32_t           speed_min = 0;
        uint32_t           speed_max = 0;
        uint32_t           brightness_min = 0;
        uint32_t           brightness_max = 0;
        uint32_t           colors_min = 0;
        uint32_t           colors_max = 0;
        uint32_t           speed = 0;
        uint32_t           brightness = 0;
        uint32_t           direction = 0;
        uint32_t           color_mode = 0;
        std::vector<Color> colors;
    };

    /**
     * @brief Individual LED description.
     */
    struct LED {
        std::string name;
        uint32_t    value = 0;
    };

    /**
     * @brief Complete description and state of an OpenRGB controller.
     */
    struct Controller {
        std::string        name;
        std::string        vendor;
        std::string        description;
        std::string        version;
        std::string        serial;
        std::string        location;
        DeviceType         type = DeviceType::Unknown;
        int32_t            active_mode = 0;
        std::vector<Mode>  modes;
        std::vector<Zone>  zones;
        std::vector<LED>   leds;
        std::vector<Color> colors;
    };

    /**
     * @brief Standalone OpenRGB TCP Client.
     *
     * Thread-safety: Individual calls on the same Client instance should be synchronized.
     * All network I/O functions report errors gracefully without throwing exceptions.
     */
    class Client {
    public:
        Client();
        ~Client();

        Client(const Client &) = delete;
        Client &operator=(const Client &) = delete;
        Client(Client &&) noexcept;
        Client &operator=(Client &&) noexcept;

        /**
         * @brief Connects to an OpenRGB server via TCP.
         * @param host Server IP or hostname (default: "127.0.0.1").
         * @param port TCP port (default: 6742).
         * @param timeout_ms Connection timeout in milliseconds.
         * @return true on success, false on failure (check GetLastError()).
         */
        bool Connect(const std::string &host = "127.0.0.1", uint16_t port = OPENRGB_DEFAULT_PORT, uint32_t timeout_ms = 3000);

        /**
         * @brief Closes the TCP connection.
         */
        void Disconnect();

        /**
         * @brief Checks if the client is currently connected.
         */
        bool IsConnected() const;

        /**
         * @brief Returns the last error description.
         */
        const std::string &GetLastError() const;

        /**
         * @brief Returns the negotiated protocol version (0 if not connected/negotiated).
         */
        uint32_t GetProtocolVersion() const;

        /**
         * @brief Negotiates protocol version with the server.
         * @param client_version Maximum protocol version supported by the client (default: 5).
         * @return Negotiated protocol version, or 0 on error.
         */
        uint32_t RequestProtocolVersion(uint32_t client_version = OPENRGB_SDK_PROTOCOL_VERSION);

        /**
         * @brief Sends client application name to the server.
         * @param name Application name string.
         * @return true on success, false on failure.
         */
        bool SetClientName(const std::string &name);

        /**
         * @brief Queries total number of controllers currently known to the server.
         * @return Controller count, or -1 on error.
         */
        int32_t RequestControllerCount();

        /**
         * @brief Retrieves complete metadata and current state for a controller.
         * @param dev_id Device index (0 to count - 1).
         * @param out_controller Controller data populated on success.
         * @return true on success, false on failure.
         */
        bool RequestControllerData(uint32_t dev_id, Controller &out_controller);

        /**
         * @brief Requests OpenRGB server to rescan hardware buses for new devices.
         * @return true on success, false on failure.
         */
        bool RescanDevices();

        /**
         * @brief Non-blocking poll for incoming server events (e.g., NET_PACKET_ID_DEVICE_LIST_UPDATED).
         * @param timeout_ms Max time to wait in milliseconds (0 for immediate poll).
         * @return Packet ID if a packet arrived (e.g. 100), 0 on timeout, or -1 on error / disconnect.
         */
        int32_t PollEvent(uint32_t timeout_ms = 0);

        /**
         * @brief High-performance bulk color flush for all LEDs on a controller.
         * Zero-copy pointer overload.
         */
        bool UpdateLEDs(uint32_t dev_id, const Color *colors, uint16_t count);

        /**
         * @brief Vector overload for UpdateLEDs.
         */
        bool UpdateLEDs(uint32_t dev_id, const std::vector<Color> &colors);

        /**
         * @brief Updates colors for a specific zone on a controller.
         */
        bool UpdateZoneLEDs(uint32_t dev_id, uint32_t zone_id, const Color *colors, uint16_t count);

        /**
         * @brief Vector overload for UpdateZoneLEDs.
         */
        bool UpdateZoneLEDs(uint32_t dev_id, uint32_t zone_id, const std::vector<Color> &colors);

        /**
         * @brief Updates a single LED color.
         */
        bool UpdateSingleLED(uint32_t dev_id, int32_t led_id, Color color);

        /**
         * @brief Switches a controller to custom mode.
         */
        bool SetCustomMode(uint32_t dev_id);

        /**
         * @brief Updates mode parameters or switches mode on a controller.
         */
        bool UpdateMode(uint32_t dev_id, int32_t mode_idx, const Mode &mode);

        /**
         * @brief Persists mode parameters to hardware NVRAM if supported.
         */
        bool SaveMode(uint32_t dev_id, int32_t mode_idx, const Mode &mode);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };

#ifdef OPENRGB_CLIENT_STANDALONE
} // namespace openrgb
#else
} // namespace Gorgon::Devices::OpenRGB::LowLevel

// Provide convenient namespace alias
namespace openrgb = Gorgon::Devices::OpenRGB::LowLevel;
#endif
