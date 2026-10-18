/**
 * @file OpenRGBClient.cpp
 * @brief Standalone OpenRGB C++17 client implementation.
 */

#include "OpenRGBClient.h"

#include <cstring>
#include <chrono>
#include <algorithm>

#if defined(_WIN32) || defined(_WIN64)
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <winsock2.h>
#   include <ws2tcpip.h>
    using socket_t = SOCKET;
#   define INVALID_SOCKET_VALUE INVALID_SOCKET
#   define SOCKET_CLOSE(s) closesocket(s)
#   define LAST_SOCK_ERR() WSAGetLastError()
#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <netinet/tcp.h>
#   include <arpa/inet.h>
#   include <netdb.h>
#   include <unistd.h>
#   include <fcntl.h>
#   include <poll.h>
#   include <cerrno>
#   include <csignal>
    using socket_t = int;
#   define INVALID_SOCKET_VALUE (-1)
#   define SOCKET_CLOSE(s) ::close(s)
#   define LAST_SOCK_ERR() (errno)
#endif

#ifdef OPENRGB_CLIENT_STANDALONE
namespace openrgb {
#else
namespace Gorgon :: Devices :: OpenRGB :: LowLevel {
#endif

namespace {

    // Little-Endian serialization helpers
    inline void WriteLE16(std::vector<uint8_t> &buf, uint16_t v) {
        buf.push_back(static_cast<uint8_t>(v & 0xFF));
        buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    }

    inline void WriteLE32(std::vector<uint8_t> &buf, uint32_t v) {
        buf.push_back(static_cast<uint8_t>(v & 0xFF));
        buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    }

    inline void WriteLE32(std::vector<uint8_t> &buf, int32_t v) {
        WriteLE32(buf, static_cast<uint32_t>(v));
    }

    inline void WriteString(std::vector<uint8_t> &buf, const std::string &str) {
        uint16_t len = static_cast<uint16_t>(str.size() + 1);
        WriteLE16(buf, len);
        buf.insert(buf.end(), str.begin(), str.end());
        buf.push_back(0); // Null terminator included on the wire
    }

    inline uint16_t ReadLE16(const uint8_t *p) {
        return static_cast<uint16_t>(p[0]) |
              (static_cast<uint16_t>(p[1]) << 8);
    }

    inline uint32_t ReadLE32(const uint8_t *p) {
        return static_cast<uint32_t>(p[0]) |
              (static_cast<uint32_t>(p[1]) << 8) |
              (static_cast<uint32_t>(p[2]) << 16) |
              (static_cast<uint32_t>(p[3]) << 24);
    }

    inline int32_t ReadLE32Signed(const uint8_t *p) {
        return static_cast<int32_t>(ReadLE32(p));
    }

    /**
     * @brief Safe binary reader with bounds checking.
     */
    class BinaryReader {
        const uint8_t *data;
        size_t size;
        size_t offset = 0;
        bool ok = true;
    public:
        BinaryReader(const uint8_t *d, size_t s) : data(d), size(s) {}

        bool HasBytes(size_t n) const {
            return ok && (offset + n <= size);
        }

        bool IsOk() const { return ok; }
        size_t Remaining() const { return (offset <= size) ? (size - offset) : 0; }
        size_t GetOffset() const { return offset; }

        uint8_t ReadU8() {
            if (!HasBytes(1)) { ok = false; return 0; }
            return data[offset++];
        }

        uint16_t ReadU16() {
            if (!HasBytes(2)) { ok = false; return 0; }
            uint16_t v = ReadLE16(data + offset);
            offset += 2;
            return v;
        }

        uint32_t ReadU32() {
            if (!HasBytes(4)) { ok = false; return 0; }
            uint32_t v = ReadLE32(data + offset);
            offset += 4;
            return v;
        }

        int32_t ReadI32() {
            return static_cast<int32_t>(ReadU32());
        }

        std::string ReadString() {
            uint16_t len = ReadU16();
            if (!ok) return {};
            if (len == 0) return {};
            if (!HasBytes(len)) { ok = false; return {}; }
            size_t str_len = (len > 0 && data[offset + len - 1] == 0) ? (len - 1) : len;
            std::string str(reinterpret_cast<const char*>(data + offset), str_len);
            offset += len;
            return str;
        }
    };

    void WriteModeData(std::vector<uint8_t> &buf, const Mode &mode, uint32_t protocol_version) {
        WriteString(buf, mode.name);
        WriteLE32(buf, mode.value);
        WriteLE32(buf, mode.flags);
        WriteLE32(buf, mode.speed_min);
        WriteLE32(buf, mode.speed_max);
        if (protocol_version >= 3) {
            WriteLE32(buf, mode.brightness_min);
            WriteLE32(buf, mode.brightness_max);
        }
        WriteLE32(buf, mode.colors_min);
        WriteLE32(buf, mode.colors_max);
        WriteLE32(buf, mode.speed);
        if (protocol_version >= 3) {
            WriteLE32(buf, mode.brightness);
        }
        WriteLE32(buf, mode.direction);
        WriteLE32(buf, mode.color_mode);
        WriteLE16(buf, static_cast<uint16_t>(mode.colors.size()));
        for (const auto &c : mode.colors) {
            WriteLE32(buf, c.value);
        }
    }

    bool ReadModeData(BinaryReader &r, Mode &mode, uint32_t protocol_version) {
        mode.name = r.ReadString();
        mode.value = r.ReadI32();
        mode.flags = r.ReadU32();
        mode.speed_min = r.ReadU32();
        mode.speed_max = r.ReadU32();
        if (protocol_version >= 3) {
            mode.brightness_min = r.ReadU32();
            mode.brightness_max = r.ReadU32();
        } else {
            mode.brightness_min = 0;
            mode.brightness_max = 100;
        }
        mode.colors_min = r.ReadU32();
        mode.colors_max = r.ReadU32();
        mode.speed = r.ReadU32();
        if (protocol_version >= 3) {
            mode.brightness = r.ReadU32();
        } else {
            mode.brightness = 100;
        }
        mode.direction = r.ReadU32();
        mode.color_mode = r.ReadU32();
        uint16_t num_colors = r.ReadU16();
        mode.colors.resize(num_colors);
        for (uint16_t i = 0; i < num_colors; ++i) {
            mode.colors[i] = Color(r.ReadU32());
        }
        return r.IsOk();
    }

    bool ReadZoneData(BinaryReader &r, Zone &zone, uint32_t protocol_version) {
        zone.name = r.ReadString();
        zone.type = static_cast<ZoneType>(r.ReadI32());
        zone.leds_min = r.ReadU32();
        zone.leds_max = r.ReadU32();
        zone.leds_count = r.ReadU32();

        if (zone.type == ZoneType::Matrix) {
            r.ReadU32(); // matrix_len
            zone.matrix_map.height = r.ReadU32();
            zone.matrix_map.width = r.ReadU32();
            uint32_t matrix_size = zone.matrix_map.height * zone.matrix_map.width;
            zone.matrix_map.matrix.resize(matrix_size);
            for (uint32_t i = 0; i < matrix_size; ++i) {
                zone.matrix_map.matrix[i] = r.ReadU32();
            }
        }

        if (protocol_version >= 4) {
            uint16_t num_segments = r.ReadU16();
            zone.segments.resize(num_segments);
            for (uint16_t s = 0; s < num_segments; ++s) {
                zone.segments[s].name = r.ReadString();
                zone.segments[s].type = r.ReadI32();
                zone.segments[s].start_idx = r.ReadU32();
                zone.segments[s].leds_count = r.ReadU32();
            }
        }
        return r.IsOk();
    }

    bool ReadControllerData(BinaryReader &r, Controller &ctrl, uint32_t protocol_version) {
        r.ReadU32(); // data_size
        ctrl.type = static_cast<DeviceType>(r.ReadI32());
        ctrl.name = r.ReadString();
        ctrl.vendor = r.ReadString();
        ctrl.description = r.ReadString();
        ctrl.version = r.ReadString();
        ctrl.serial = r.ReadString();
        ctrl.location = r.ReadString();

        uint16_t num_modes = r.ReadU16();
        ctrl.active_mode = r.ReadI32();
        ctrl.modes.resize(num_modes);
        for (uint16_t m = 0; m < num_modes; ++m) {
            if (!ReadModeData(r, ctrl.modes[m], protocol_version)) return false;
        }

        uint16_t num_zones = r.ReadU16();
        ctrl.zones.resize(num_zones);
        for (uint16_t z = 0; z < num_zones; ++z) {
            if (!ReadZoneData(r, ctrl.zones[z], protocol_version)) return false;
        }

        uint16_t num_leds = r.ReadU16();
        ctrl.leds.resize(num_leds);
        for (uint16_t l = 0; l < num_leds; ++l) {
            ctrl.leds[l].name = r.ReadString();
            ctrl.leds[l].value = r.ReadU32();
        }

        uint16_t num_colors = r.ReadU16();
        ctrl.colors.resize(num_colors);
        for (uint16_t c = 0; c < num_colors; ++c) {
            ctrl.colors[c] = Color(r.ReadU32());
        }

        return r.IsOk();
    }

#if defined(_WIN32) || defined(_WIN64)
    struct WinsockInit {
        WinsockInit() {
            WSADATA wsa;
            WSAStartup(MAKEWORD(2, 2), &wsa);
        }
        ~WinsockInit() {
            WSACleanup();
        }
    };
    void EnsureSocketsInitialized() {
        static WinsockInit s_winsock;
    }
#else
    void EnsureSocketsInitialized() {
        static bool s_ignored = false;
        if (!s_ignored) {
            std::signal(SIGPIPE, SIG_IGN);
            s_ignored = true;
        }
    }
#endif

} // anonymous namespace

struct Client::Impl {
    socket_t sock = INVALID_SOCKET_VALUE;
    uint32_t protocol_version = 0;
    std::string last_error;

    Impl() {
        EnsureSocketsInitialized();
    }

    ~Impl() {
        Close();
    }

    void Close() {
        if (sock != INVALID_SOCKET_VALUE) {
            SOCKET_CLOSE(sock);
            sock = INVALID_SOCKET_VALUE;
        }
        protocol_version = 0;
    }

    bool Send(const void *data, size_t len) {
        const char *ptr = reinterpret_cast<const char*>(data);
        size_t remaining = len;
        while (remaining > 0) {
#if defined(_WIN32) || defined(_WIN64)
            int sent = ::send(sock, ptr, static_cast<int>(remaining), 0);
#else
            ssize_t sent = ::send(sock, ptr, remaining, MSG_NOSIGNAL);
#endif
            if (sent <= 0) {
                last_error = "Socket send error: " + std::to_string(LAST_SOCK_ERR());
                Close();
                return false;
            }
            ptr += sent;
            remaining -= static_cast<size_t>(sent);
        }
        return true;
    }

    bool Recv(void *data, size_t len) {
        char *ptr = reinterpret_cast<char*>(data);
        size_t remaining = len;
        while (remaining > 0) {
#if defined(_WIN32) || defined(_WIN64)
            int recvd = ::recv(sock, ptr, static_cast<int>(remaining), 0);
#else
            ssize_t recvd = ::recv(sock, ptr, remaining, 0);
#endif
            if (recvd <= 0) {
                last_error = "Socket recv error or remote disconnect: " + std::to_string(LAST_SOCK_ERR());
                Close();
                return false;
            }
            ptr += recvd;
            remaining -= static_cast<size_t>(recvd);
        }
        return true;
    }

    bool SendPacket(uint32_t dev_id, uint32_t pkt_id, const void *payload, uint32_t payload_size) {
        if (sock == INVALID_SOCKET_VALUE) {
            last_error = "Socket is not connected";
            return false;
        }

        uint8_t header[16];
        header[0] = 'O'; header[1] = 'R'; header[2] = 'G'; header[3] = 'B';
        header[4] = dev_id & 0xFF; header[5] = (dev_id >> 8) & 0xFF;
        header[6] = (dev_id >> 16) & 0xFF; header[7] = (dev_id >> 24) & 0xFF;
        header[8] = pkt_id & 0xFF; header[9] = (pkt_id >> 8) & 0xFF;
        header[10] = (pkt_id >> 16) & 0xFF; header[11] = (pkt_id >> 24) & 0xFF;
        header[12] = payload_size & 0xFF; header[13] = (payload_size >> 8) & 0xFF;
        header[14] = (payload_size >> 16) & 0xFF; header[15] = (payload_size >> 24) & 0xFF;

        if (!Send(header, sizeof(header))) return false;
        if (payload_size > 0 && payload != nullptr) {
            if (!Send(payload, payload_size)) return false;
        }
        return true;
    }

    bool ReadPacket(uint32_t &out_dev_id, uint32_t &out_pkt_id, std::vector<uint8_t> &out_payload, uint32_t timeout_ms) {
        if (sock == INVALID_SOCKET_VALUE) {
            last_error = "Socket is not connected";
            return false;
        }

        if (timeout_ms > 0) {
#if defined(_WIN32) || defined(_WIN64)
            fd_set rset;
            FD_ZERO(&rset);
            FD_SET(sock, &rset);
            struct timeval tv;
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;
            int sel = select(static_cast<int>(sock + 1), &rset, nullptr, nullptr, &tv);
            if (sel <= 0) {
                last_error = "Read packet timed out";
                return false;
            }
#else
            struct pollfd pfd;
            pfd.fd = sock;
            pfd.events = POLLIN;
            pfd.revents = 0;
            int p = poll(&pfd, 1, static_cast<int>(timeout_ms));
            if (p <= 0) {
                last_error = "Read packet timed out";
                return false;
            }
#endif
        }

        uint8_t header[16];
        if (!Recv(header, sizeof(header))) return false;

        if (header[0] != 'O' || header[1] != 'R' || header[2] != 'G' || header[3] != 'B') {
            last_error = "Invalid packet magic (expected 'ORGB')";
            Close();
            return false;
        }

        out_dev_id = ReadLE32(header + 4);
        out_pkt_id = ReadLE32(header + 8);
        uint32_t pkt_size = ReadLE32(header + 12);

        out_payload.resize(pkt_size);
        if (pkt_size > 0) {
            if (!Recv(out_payload.data(), pkt_size)) {
                return false;
            }
        }
        return true;
    }
};

Client::Client() : impl(std::make_unique<Impl>()) {}
Client::~Client() = default;

Client::Client(Client &&) noexcept = default;
Client &Client::operator=(Client &&) noexcept = default;

bool Client::Connect(const std::string &host, uint16_t port, uint32_t timeout_ms) {
    Disconnect();

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *result = nullptr;
    std::string port_str = std::to_string(port);
    int status = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
    if (status != 0 || result == nullptr) {
        impl->last_error = "Failed to resolve host: " + host;
        return false;
    }

    socket_t s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (s == INVALID_SOCKET_VALUE) {
        impl->last_error = "Failed to create socket: " + std::to_string(LAST_SOCK_ERR());
        freeaddrinfo(result);
        return false;
    }

    // Set non-blocking for connect timeout
#if defined(_WIN32) || defined(_WIN64)
    u_long non_blocking = 1;
    ioctlsocket(s, FIONBIO, &non_blocking);
#else
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif

    int conn_res = ::connect(s, result->ai_addr, static_cast<socklen_t>(result->ai_addrlen));
    freeaddrinfo(result);

    bool connected = false;
    if (conn_res == 0) {
        connected = true;
    } else {
#if defined(_WIN32) || defined(_WIN64)
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            fd_set wset, eset;
            FD_ZERO(&wset);
            FD_ZERO(&eset);
            FD_SET(s, &wset);
            FD_SET(s, &eset);
            struct timeval tv;
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;
            int sel = select(static_cast<int>(s + 1), nullptr, &wset, &eset, &tv);
            if (sel > 0 && FD_ISSET(s, &wset) && !FD_ISSET(s, &eset)) {
                int err = 0;
                int errlen = sizeof(err);
                getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &errlen);
                if (err == 0) connected = true;
            }
        }
#else
        if (errno == EINPROGRESS) {
            struct pollfd pfd;
            pfd.fd = s;
            pfd.events = POLLOUT;
            pfd.revents = 0;
            int p = poll(&pfd, 1, static_cast<int>(timeout_ms));
            if (p > 0 && !(pfd.revents & (POLLERR | POLLHUP | POLLNVAL))) {
                int err = 0;
                socklen_t errlen = sizeof(err);
                getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &errlen);
                if (err == 0) connected = true;
            }
        }
#endif
    }

    if (!connected) {
        impl->last_error = "Connection to OpenRGB server timed out or was refused (" + host + ":" + port_str + ")";
        SOCKET_CLOSE(s);
        return false;
    }

    // Restore blocking mode
#if defined(_WIN32) || defined(_WIN64)
    non_blocking = 0;
    ioctlsocket(s, FIONBIO, &non_blocking);
#else
    fcntl(s, F_SETFL, flags & ~O_NONBLOCK);
#endif

    // Disable Nagle's algorithm for immediate dispatch of lighting commands
    int nodelay = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&nodelay), sizeof(nodelay));

    impl->sock = s;
    impl->last_error.clear();
    return true;
}

void Client::Disconnect() {
    impl->Close();
}

bool Client::IsConnected() const {
    return impl->sock != INVALID_SOCKET_VALUE;
}

const std::string &Client::GetLastError() const {
    return impl->last_error;
}

uint32_t Client::GetProtocolVersion() const {
    return impl->protocol_version;
}

uint32_t Client::RequestProtocolVersion(uint32_t client_version) {
    if (!IsConnected()) return 0;

    uint8_t payload[4];
    payload[0] = client_version & 0xFF;
    payload[1] = (client_version >> 8) & 0xFF;
    payload[2] = (client_version >> 16) & 0xFF;
    payload[3] = (client_version >> 24) & 0xFF;

    if (!impl->SendPacket(0, NET_PACKET_ID_REQUEST_PROTOCOL_VERSION, payload, sizeof(payload))) {
        return 0;
    }

    uint32_t resp_dev_id = 0, resp_pkt_id = 0;
    std::vector<uint8_t> resp_payload;
    if (!impl->ReadPacket(resp_dev_id, resp_pkt_id, resp_payload, 3000)) {
        impl->last_error = "Timeout waiting for protocol version negotiation response";
        return 0;
    }

    if (resp_pkt_id != NET_PACKET_ID_REQUEST_PROTOCOL_VERSION || resp_payload.size() < 4) {
        impl->last_error = "Invalid protocol version response";
        return 0;
    }

    uint32_t server_version = ReadLE32(resp_payload.data());
    impl->protocol_version = std::min(client_version, server_version);
    return impl->protocol_version;
}

bool Client::SetClientName(const std::string &name) {
    if (!IsConnected()) return false;
    std::vector<uint8_t> payload;
    WriteString(payload, name);
    return impl->SendPacket(0, NET_PACKET_ID_SET_CLIENT_NAME, payload.data(), static_cast<uint32_t>(payload.size()));
}

int32_t Client::RequestControllerCount() {
    if (!IsConnected()) return -1;

    if (!impl->SendPacket(0, NET_PACKET_ID_REQUEST_CONTROLLER_COUNT, nullptr, 0)) {
        return -1;
    }

    uint32_t resp_dev_id = 0, resp_pkt_id = 0;
    std::vector<uint8_t> resp_payload;
    if (!impl->ReadPacket(resp_dev_id, resp_pkt_id, resp_payload, 3000)) {
        impl->last_error = "Timeout waiting for controller count response";
        return -1;
    }

    if (resp_pkt_id != NET_PACKET_ID_REQUEST_CONTROLLER_COUNT || resp_payload.size() < 4) {
        impl->last_error = "Invalid controller count response";
        return -1;
    }

    return static_cast<int32_t>(ReadLE32(resp_payload.data()));
}

bool Client::RequestControllerData(uint32_t dev_id, Controller &out_controller) {
    if (!IsConnected()) return false;

    uint8_t payload[4];
    payload[0] = impl->protocol_version & 0xFF;
    payload[1] = (impl->protocol_version >> 8) & 0xFF;
    payload[2] = (impl->protocol_version >> 16) & 0xFF;
    payload[3] = (impl->protocol_version >> 24) & 0xFF;

    if (!impl->SendPacket(dev_id, NET_PACKET_ID_REQUEST_CONTROLLER_DATA, payload, sizeof(payload))) {
        return false;
    }

    uint32_t resp_dev_id = 0, resp_pkt_id = 0;
    std::vector<uint8_t> resp_payload;
    if (!impl->ReadPacket(resp_dev_id, resp_pkt_id, resp_payload, 5000)) {
        impl->last_error = "Timeout waiting for controller data response";
        return false;
    }

    if (resp_pkt_id != NET_PACKET_ID_REQUEST_CONTROLLER_DATA || resp_dev_id != dev_id) {
        impl->last_error = "Mismatched packet in controller data response";
        return false;
    }

    BinaryReader r(resp_payload.data(), resp_payload.size());
    if (!ReadControllerData(r, out_controller, impl->protocol_version)) {
        impl->last_error = "Failed to deserialize controller data (corrupted or incomplete response)";
        return false;
    }

    return true;
}

bool Client::RescanDevices() {
    if (!IsConnected()) return false;
    return impl->SendPacket(0, NET_PACKET_ID_REQUEST_RESCAN_DEVICES, nullptr, 0);
}

int32_t Client::PollEvent(uint32_t timeout_ms) {
    if (!IsConnected()) return -1;

#if defined(_WIN32) || defined(_WIN64)
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(impl->sock, &rset);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int sel = select(static_cast<int>(impl->sock + 1), &rset, nullptr, nullptr, &tv);
    if (sel < 0) {
        impl->Close();
        return -1;
    }
    if (sel == 0) return 0;
#else
    struct pollfd pfd;
    pfd.fd = impl->sock;
    pfd.events = POLLIN;
    pfd.revents = 0;
    int p = poll(&pfd, 1, static_cast<int>(timeout_ms));
    if (p < 0) {
        impl->Close();
        return -1;
    }
    if (p == 0) return 0;
    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        impl->Close();
        return -1;
    }
#endif

    uint32_t dev_id = 0, pkt_id = 0;
    std::vector<uint8_t> payload;
    if (!impl->ReadPacket(dev_id, pkt_id, payload, 0)) {
        return -1;
    }

    return static_cast<int32_t>(pkt_id);
}

bool Client::UpdateLEDs(uint32_t dev_id, const Color *colors, uint16_t count) {
    if (!IsConnected() || colors == nullptr) return false;

    uint32_t data_size = 2 + static_cast<uint32_t>(count) * 4;
    uint32_t pkt_size = 4 + data_size;

    uint8_t combined_hdr[22];
    // 16-byte Packet Header
    combined_hdr[0] = 'O'; combined_hdr[1] = 'R'; combined_hdr[2] = 'G'; combined_hdr[3] = 'B';
    combined_hdr[4] = dev_id & 0xFF; combined_hdr[5] = (dev_id >> 8) & 0xFF;
    combined_hdr[6] = (dev_id >> 16) & 0xFF; combined_hdr[7] = (dev_id >> 24) & 0xFF;
    combined_hdr[8] = NET_PACKET_ID_RGBCONTROLLER_UPDATELEDS & 0xFF;
    combined_hdr[9] = (NET_PACKET_ID_RGBCONTROLLER_UPDATELEDS >> 8) & 0xFF;
    combined_hdr[10] = (NET_PACKET_ID_RGBCONTROLLER_UPDATELEDS >> 16) & 0xFF;
    combined_hdr[11] = (NET_PACKET_ID_RGBCONTROLLER_UPDATELEDS >> 24) & 0xFF;
    combined_hdr[12] = pkt_size & 0xFF;
    combined_hdr[13] = (pkt_size >> 8) & 0xFF;
    combined_hdr[14] = (pkt_size >> 16) & 0xFF;
    combined_hdr[15] = (pkt_size >> 24) & 0xFF;

    // data_size
    combined_hdr[16] = data_size & 0xFF;
    combined_hdr[17] = (data_size >> 8) & 0xFF;
    combined_hdr[18] = (data_size >> 16) & 0xFF;
    combined_hdr[19] = (data_size >> 24) & 0xFF;

    // num_colors
    combined_hdr[20] = count & 0xFF;
    combined_hdr[21] = (count >> 8) & 0xFF;

    if (!impl->Send(combined_hdr, sizeof(combined_hdr))) return false;
    if (count > 0) {
        if (!impl->Send(colors, count * sizeof(Color))) return false;
    }
    return true;
}

bool Client::UpdateLEDs(uint32_t dev_id, const std::vector<Color> &colors) {
    return UpdateLEDs(dev_id, colors.data(), static_cast<uint16_t>(colors.size()));
}

bool Client::UpdateZoneLEDs(uint32_t dev_id, uint32_t zone_id, const Color *colors, uint16_t count) {
    if (!IsConnected() || colors == nullptr) return false;

    uint32_t data_size = 4 + 2 + static_cast<uint32_t>(count) * 4;
    uint32_t pkt_size = 4 + data_size;

    uint8_t combined_hdr[26];
    // 16-byte Packet Header
    combined_hdr[0] = 'O'; combined_hdr[1] = 'R'; combined_hdr[2] = 'G'; combined_hdr[3] = 'B';
    combined_hdr[4] = dev_id & 0xFF; combined_hdr[5] = (dev_id >> 8) & 0xFF;
    combined_hdr[6] = (dev_id >> 16) & 0xFF; combined_hdr[7] = (dev_id >> 24) & 0xFF;
    combined_hdr[8] = NET_PACKET_ID_RGBCONTROLLER_UPDATEZONELEDS & 0xFF;
    combined_hdr[9] = (NET_PACKET_ID_RGBCONTROLLER_UPDATEZONELEDS >> 8) & 0xFF;
    combined_hdr[10] = (NET_PACKET_ID_RGBCONTROLLER_UPDATEZONELEDS >> 16) & 0xFF;
    combined_hdr[11] = (NET_PACKET_ID_RGBCONTROLLER_UPDATEZONELEDS >> 24) & 0xFF;
    combined_hdr[12] = pkt_size & 0xFF;
    combined_hdr[13] = (pkt_size >> 8) & 0xFF;
    combined_hdr[14] = (pkt_size >> 16) & 0xFF;
    combined_hdr[15] = (pkt_size >> 24) & 0xFF;

    // data_size
    combined_hdr[16] = data_size & 0xFF;
    combined_hdr[17] = (data_size >> 8) & 0xFF;
    combined_hdr[18] = (data_size >> 16) & 0xFF;
    combined_hdr[19] = (data_size >> 24) & 0xFF;

    // zone_idx
    combined_hdr[20] = zone_id & 0xFF;
    combined_hdr[21] = (zone_id >> 8) & 0xFF;
    combined_hdr[22] = (zone_id >> 16) & 0xFF;
    combined_hdr[23] = (zone_id >> 24) & 0xFF;

    // num_colors
    combined_hdr[24] = count & 0xFF;
    combined_hdr[25] = (count >> 8) & 0xFF;

    if (!impl->Send(combined_hdr, sizeof(combined_hdr))) return false;
    if (count > 0) {
        if (!impl->Send(colors, count * sizeof(Color))) return false;
    }
    return true;
}

bool Client::UpdateZoneLEDs(uint32_t dev_id, uint32_t zone_id, const std::vector<Color> &colors) {
    return UpdateZoneLEDs(dev_id, zone_id, colors.data(), static_cast<uint16_t>(colors.size()));
}

bool Client::UpdateSingleLED(uint32_t dev_id, int32_t led_id, Color color) {
    if (!IsConnected()) return false;

    uint8_t packet[24];
    // 16-byte Header
    packet[0] = 'O'; packet[1] = 'R'; packet[2] = 'G'; packet[3] = 'B';
    packet[4] = dev_id & 0xFF; packet[5] = (dev_id >> 8) & 0xFF;
    packet[6] = (dev_id >> 16) & 0xFF; packet[7] = (dev_id >> 24) & 0xFF;
    packet[8] = NET_PACKET_ID_RGBCONTROLLER_UPDATESINGLELED & 0xFF;
    packet[9] = (NET_PACKET_ID_RGBCONTROLLER_UPDATESINGLELED >> 8) & 0xFF;
    packet[10] = (NET_PACKET_ID_RGBCONTROLLER_UPDATESINGLELED >> 16) & 0xFF;
    packet[11] = (NET_PACKET_ID_RGBCONTROLLER_UPDATESINGLELED >> 24) & 0xFF;
    packet[12] = 8; packet[13] = 0; packet[14] = 0; packet[15] = 0;

    // Payload: led_idx (int32_t), color (uint32_t)
    uint32_t led_u = static_cast<uint32_t>(led_id);
    packet[16] = led_u & 0xFF; packet[17] = (led_u >> 8) & 0xFF;
    packet[18] = (led_u >> 16) & 0xFF; packet[19] = (led_u >> 24) & 0xFF;

    packet[20] = color.value & 0xFF; packet[21] = (color.value >> 8) & 0xFF;
    packet[22] = (color.value >> 16) & 0xFF; packet[23] = (color.value >> 24) & 0xFF;

    return impl->Send(packet, sizeof(packet));
}

bool Client::SetCustomMode(uint32_t dev_id) {
    if (!IsConnected()) return false;
    return impl->SendPacket(dev_id, NET_PACKET_ID_RGBCONTROLLER_SETCUSTOMMODE, nullptr, 0);
}

bool Client::UpdateMode(uint32_t dev_id, int32_t mode_idx, const Mode &mode) {
    if (!IsConnected()) return false;

    std::vector<uint8_t> mode_bytes;
    WriteModeData(mode_bytes, mode, impl->protocol_version);

    uint32_t data_size = 4 + static_cast<uint32_t>(mode_bytes.size());
    std::vector<uint8_t> payload;
    payload.reserve(4 + data_size);

    WriteLE32(payload, data_size);
    WriteLE32(payload, mode_idx);
    payload.insert(payload.end(), mode_bytes.begin(), mode_bytes.end());

    return impl->SendPacket(dev_id, NET_PACKET_ID_RGBCONTROLLER_UPDATEMODE, payload.data(), static_cast<uint32_t>(payload.size()));
}

bool Client::SaveMode(uint32_t dev_id, int32_t mode_idx, const Mode &mode) {
    if (!IsConnected()) return false;

    std::vector<uint8_t> mode_bytes;
    WriteModeData(mode_bytes, mode, impl->protocol_version);

    uint32_t data_size = 4 + static_cast<uint32_t>(mode_bytes.size());
    std::vector<uint8_t> payload;
    payload.reserve(4 + data_size);

    WriteLE32(payload, data_size);
    WriteLE32(payload, mode_idx);
    payload.insert(payload.end(), mode_bytes.begin(), mode_bytes.end());

    return impl->SendPacket(dev_id, NET_PACKET_ID_RGBCONTROLLER_SAVEMODE, payload.data(), static_cast<uint32_t>(payload.size()));
}

#ifdef OPENRGB_CLIENT_STANDALONE
} // namespace openrgb
#else
} // namespace Gorgon::Devices::OpenRGB::LowLevel
#endif
