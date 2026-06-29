#include "emrtc/canopen.hpp"

#include "emrtc/process_image_mapping.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <sstream>
#include <utility>

#if defined(EMRTC_HAS_SOCKETCAN) && defined(__linux__)
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace emrtc {
namespace {

void write_u16(std::array<std::uint8_t, 8>& data,
               std::size_t offset,
               std::uint16_t value) {
    data[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    data[offset + 1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

void write_i32(std::array<std::uint8_t, 8>& data,
               std::size_t offset,
               std::int32_t value) {
    const auto encoded = static_cast<std::uint32_t>(value);
    for (std::size_t index = 0; index < 4U; ++index) {
        data[offset + index] =
            static_cast<std::uint8_t>((encoded >> (index * 8U)) & 0xFFU);
    }
}

std::uint16_t read_u16(const std::array<std::uint8_t, 8>& data,
                       std::size_t offset) {
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(data[offset]) |
        (static_cast<std::uint16_t>(data[offset + 1U]) << 8U));
}

std::int32_t read_i32(const std::array<std::uint8_t, 8>& data,
                      std::size_t offset) {
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < 4U; ++index) {
        value |= static_cast<std::uint32_t>(data[offset + index]) << (index * 8U);
    }
    return static_cast<std::int32_t>(value);
}

CanFrame nmt_frame(std::uint8_t command, std::uint8_t node_id) {
    CanFrame frame;
    frame.id = 0U;
    frame.dlc = 2U;
    frame.data[0] = command;
    frame.data[1] = node_id;
    return frame;
}

bool compile_flag_enabled() {
#ifdef EMRTC_ENABLE_SOCKETCAN
    return true;
#else
    return false;
#endif
}

bool dependency_available() {
#if defined(EMRTC_HAS_SOCKETCAN) && defined(__linux__)
    return true;
#else
    return false;
#endif
}

}  // namespace

struct SocketCanTransport::Impl {
#if defined(EMRTC_HAS_SOCKETCAN) && defined(__linux__)
    int socket_fd{-1};
#endif
    bool open{false};
};

SocketCanTransport::SocketCanTransport() : impl_(std::make_unique<Impl>()) {}

SocketCanTransport::~SocketCanTransport() {
    close();
}

bool SocketCanTransport::open(const std::string& interface_name, std::string& detail) {
    close();
#if defined(EMRTC_HAS_SOCKETCAN) && defined(__linux__)
    if (interface_name.empty() || interface_name.size() >= IFNAMSIZ) {
        detail = "SocketCAN interface name is empty or too long";
        return false;
    }
    const int socket_fd = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd < 0) {
        detail = std::string("socket(PF_CAN) failed: ") + std::strerror(errno);
        return false;
    }

    struct ifreq request {};
    std::memcpy(request.ifr_name, interface_name.c_str(), interface_name.size() + 1U);
    if (::ioctl(socket_fd, SIOCGIFINDEX, &request) < 0) {
        detail = "SIOCGIFINDEX failed for " + interface_name + ": " + std::strerror(errno);
        ::close(socket_fd);
        return false;
    }

    struct sockaddr_can address {};
    address.can_family = AF_CAN;
    address.can_ifindex = request.ifr_ifindex;
    if (::bind(socket_fd,
               reinterpret_cast<const struct sockaddr*>(&address),
               sizeof(address)) < 0) {
        detail = "bind(AF_CAN) failed for " + interface_name + ": " + std::strerror(errno);
        ::close(socket_fd);
        return false;
    }

    impl_->socket_fd = socket_fd;
    impl_->open = true;
    detail = "SocketCAN raw socket bound to " + interface_name;
    return true;
#else
    (void)interface_name;
    detail = "SocketCAN transport is unavailable in this build";
    return false;
#endif
}

void SocketCanTransport::close() {
#if defined(EMRTC_HAS_SOCKETCAN) && defined(__linux__)
    if (impl_ && impl_->socket_fd >= 0) {
        ::close(impl_->socket_fd);
        impl_->socket_fd = -1;
    }
#endif
    if (impl_) impl_->open = false;
}

bool SocketCanTransport::send(const CanFrame& frame, std::string& detail) {
    if (!impl_ || !impl_->open) {
        detail = "SocketCAN transport is not open";
        return false;
    }
    if (frame.id > 0x7FFU || frame.dlc > 8U) {
        detail = "CAN frame exceeds classical 11-bit/8-byte limits";
        return false;
    }
#if defined(EMRTC_HAS_SOCKETCAN) && defined(__linux__)
    struct can_frame native {};
    native.can_id = frame.id;
    native.can_dlc = frame.dlc;
    std::copy_n(frame.data.begin(), frame.dlc, native.data);
    const auto written = ::write(impl_->socket_fd, &native, sizeof(native));
    if (written != static_cast<ssize_t>(sizeof(native))) {
        detail = std::string("SocketCAN write failed: ") + std::strerror(errno);
        return false;
    }
    detail = "ok";
    return true;
#else
    (void)frame;
    detail = "SocketCAN transport is unavailable in this build";
    return false;
#endif
}

CanReceiveStatus SocketCanTransport::receive(CanFrame& frame,
                                             std::uint32_t timeout_us,
                                             std::string& detail) {
    if (!impl_ || !impl_->open) {
        detail = "SocketCAN transport is not open";
        return CanReceiveStatus::Error;
    }
#if defined(EMRTC_HAS_SOCKETCAN) && defined(__linux__)
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(impl_->socket_fd, &read_set);
    struct timeval timeout {};
    timeout.tv_sec = static_cast<long>(timeout_us / 1'000'000U);
    timeout.tv_usec = static_cast<long>(timeout_us % 1'000'000U);
    const int ready = ::select(impl_->socket_fd + 1, &read_set, nullptr, nullptr, &timeout);
    if (ready == 0) {
        detail = "timeout";
        return CanReceiveStatus::Timeout;
    }
    if (ready < 0) {
        detail = std::string("SocketCAN select failed: ") + std::strerror(errno);
        return CanReceiveStatus::Error;
    }

    struct can_frame native {};
    const auto received = ::read(impl_->socket_fd, &native, sizeof(native));
    if (received != static_cast<ssize_t>(sizeof(native))) {
        detail = std::string("SocketCAN read failed or returned a short frame: ") +
                 std::strerror(errno);
        return CanReceiveStatus::Error;
    }
    if ((native.can_id & (CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_ERR_FLAG)) != 0U ||
        native.can_dlc > 8U) {
        detail = "extended, remote, error, or oversized CAN frame rejected";
        return CanReceiveStatus::Error;
    }
    frame = {};
    frame.id = native.can_id & CAN_SFF_MASK;
    frame.dlc = native.can_dlc;
    std::copy_n(native.data, native.can_dlc, frame.data.begin());
    detail = "ok";
    return CanReceiveStatus::Frame;
#else
    (void)frame;
    (void)timeout_us;
    detail = "SocketCAN transport is unavailable in this build";
    return CanReceiveStatus::Error;
#endif
}

bool SocketCanTransport::is_open() const {
    return impl_ && impl_->open;
}

CanFrame encode_canopen_rpdo1(const ProcessImage& image,
                              const SocketCanCanOpenConfig& config) {
    const auto wire = encode_cia402_wire_image(image, config.counts_per_unit);
    CanFrame frame;
    frame.id = config.rpdo1_cob_id;
    frame.dlc = 6U;
    write_u16(frame.data, 0U, wire.controlword);
    write_i32(frame.data, 2U, wire.target_position_counts);
    return frame;
}

CanFrame encode_canopen_rpdo2(const ProcessImage& image,
                              const SocketCanCanOpenConfig& config) {
    const auto wire = encode_cia402_wire_image(image, config.counts_per_unit);
    CanFrame frame;
    frame.id = config.rpdo2_cob_id;
    frame.dlc = 4U;
    write_i32(frame.data, 0U, wire.target_velocity_counts);
    return frame;
}

AdapterContractResult decode_canopen_tpdo(const CanFrame& frame,
                                          const SocketCanCanOpenConfig& config,
                                          ProcessImage& image,
                                          bool& received_tpdo1,
                                          bool& received_tpdo2) {
    auto wire = encode_cia402_wire_image(image, config.counts_per_unit);
    if (frame.id == config.tpdo1_cob_id) {
        if (frame.dlc != 6U) {
            return {false, FaultCode::InvalidDriveTransition,
                    "CANopen TPDO1 must contain statusword and actual position (6 bytes)"};
        }
        wire.statusword = read_u16(frame.data, 0U);
        wire.actual_position_counts = read_i32(frame.data, 2U);
        wire.drive_fault =
            (wire.statusword & 0x0008U) != 0U ? FaultCode::DriveFault : FaultCode::None;
        received_tpdo1 = true;
    } else if (frame.id == config.tpdo2_cob_id) {
        if (frame.dlc != 4U) {
            return {false, FaultCode::InvalidDriveTransition,
                    "CANopen TPDO2 must contain actual velocity (4 bytes)"};
        }
        wire.actual_velocity_counts = read_i32(frame.data, 0U);
        received_tpdo2 = true;
    } else {
        return {false, FaultCode::InvalidDriveTransition,
                "CAN frame does not match configured TPDO identifiers"};
    }
    image = decode_cia402_wire_image(wire, config.counts_per_unit);
    return {};
}

SocketCanCanOpenBackend::SocketCanCanOpenBackend()
    : owned_transport_(std::make_unique<SocketCanTransport>()),
      transport_(owned_transport_.get()) {
    diagnostics_.online = false;
    diagnostics_.heartbeat_ok = false;
    status_ = {false,
               compile_flag_enabled(),
               dependency_available(),
               "SocketCAN CANopen",
               dependency_available()
                   ? "SocketCAN dependency available; adapter requires explicit configuration"
                   : "SocketCAN dependency unavailable in this build"};
}

SocketCanCanOpenBackend::SocketCanCanOpenBackend(ICanTransport& transport)
    : transport_(&transport), injected_transport_(true) {
    diagnostics_.online = false;
    diagnostics_.heartbeat_ok = false;
    status_ = {false, true, true, "SocketCAN CANopen",
               "injected CAN transport requires explicit configuration"};
}

SocketCanCanOpenBackend::~SocketCanCanOpenBackend() {
    shutdown();
}

AdapterConfigurationResult SocketCanCanOpenBackend::fail_configuration(
    const std::string& detail) {
    status_.configured = false;
    status_.detail = detail;
    diagnostics_.online = false;
    diagnostics_.heartbeat_ok = false;
    return {false, 0, 0, 0, 0, detail};
}

AdapterConfigurationResult SocketCanCanOpenBackend::configure(
    const SocketCanCanOpenConfig& config) {
    shutdown();
    RuntimeAdapterConfig runtime;
    runtime.backend = RuntimeAdapterBackend::SocketCanCanOpen;
    runtime.socketcan = config;
    try {
        validate_runtime_adapter_config(runtime);
    } catch (const std::exception& error) {
        return fail_configuration(error.what());
    }
    if (!injected_transport_ && !compile_flag_enabled()) {
        return fail_configuration("SocketCAN build flag is disabled");
    }
    if (!injected_transport_ && !dependency_available()) {
        return fail_configuration("SocketCAN dependency is unavailable");
    }

    std::string detail;
    if (!transport_->open(config.interface_name, detail)) {
        return fail_configuration(detail);
    }
    if (!transport_->send(nmt_frame(0x01U, config.node_id), detail)) {
        transport_->close();
        return fail_configuration("CANopen NMT start failed: " + detail);
    }

    config_ = config;
    diagnostics_ = {};
    diagnostics_.tx_packets = 1U;
    diagnostics_.online = false;
    diagnostics_.heartbeat_ok = false;
    status_.configured = true;
    status_.detail = "SocketCAN open and CANopen NMT start transmitted; awaiting TPDO and heartbeat";
    have_heartbeat_ = false;
    previous_online_ = false;
    return {true, 1U, 10U, 10U, 0, status_.detail};
}

void SocketCanCanOpenBackend::shutdown() {
    if (transport_ && transport_->is_open()) {
        std::string ignored;
        if (status_.configured) {
            (void)transport_->send(nmt_frame(0x02U, config_.node_id), ignored);
        }
        transport_->close();
    }
    diagnostics_ = {};
    diagnostics_.online = false;
    diagnostics_.heartbeat_ok = false;
    status_.configured = false;
    status_.detail = injected_transport_
                         ? "injected CAN transport requires explicit configuration"
                         : (dependency_available()
                                ? "SocketCAN dependency available; adapter requires explicit configuration"
                                : "SocketCAN dependency unavailable in this build");
    have_heartbeat_ = false;
    previous_online_ = false;
}

bool SocketCanCanOpenBackend::handle_receive_frame(const CanFrame& frame,
                                                   CycleIndex cycle,
                                                   ProcessImage& image,
                                                   bool& received_tpdo1,
                                                   bool& received_tpdo2) {
    if (frame.id == config_.heartbeat_cob_id) {
        if (frame.dlc != 1U) {
            diagnostics_.invalid_response_count++;
            return false;
        }
        have_heartbeat_ = true;
        diagnostics_.last_heartbeat_cycle = cycle;
        diagnostics_.heartbeat_ok = frame.data[0] == 0x05U;
        return true;
    }
    if (frame.id != config_.tpdo1_cob_id && frame.id != config_.tpdo2_cob_id) {
        return true;
    }
    const auto result = decode_canopen_tpdo(
        frame, config_, image, received_tpdo1, received_tpdo2);
    if (!result.ok) {
        diagnostics_.invalid_response_count++;
        return false;
    }
    return true;
}

BusExchangeResult SocketCanCanOpenBackend::exchange(
    CycleIndex cycle,
    std::vector<ProcessImage>& images) {
    if (!status_.configured || !transport_ || !transport_->is_open() || images.size() != 1U) {
        diagnostics_.dropout_count++;
        diagnostics_.online = false;
        diagnostics_.heartbeat_ok = false;
        const bool invalid = images.size() != 1U;
        if (invalid) diagnostics_.invalid_response_count++;
        return {false, false, invalid};
    }

    std::string detail;
    const auto rpdo1 = encode_canopen_rpdo1(images.front(), config_);
    const auto rpdo2 = encode_canopen_rpdo2(images.front(), config_);
    if (!transport_->send(rpdo1, detail) || !transport_->send(rpdo2, detail)) {
        diagnostics_.dropout_count++;
        diagnostics_.online = false;
        diagnostics_.heartbeat_ok = false;
        if (previous_online_) diagnostics_.cable_disconnect_events++;
        previous_online_ = false;
        status_.detail = "SocketCAN transmit failed: " + detail;
        return {false, false, false};
    }
    diagnostics_.tx_packets += 2U;

    bool received_tpdo1 = false;
    bool received_tpdo2 = false;
    for (int attempt = 0; attempt < 8; ++attempt) {
        CanFrame frame;
        const std::uint32_t timeout_us =
            received_tpdo1 && received_tpdo2 ? 0U : config_.receive_timeout_us;
        const auto receive_status =
            transport_->receive(frame, timeout_us, detail);
        if (receive_status == CanReceiveStatus::Timeout) break;
        if (receive_status == CanReceiveStatus::Error) {
            diagnostics_.dropout_count++;
            diagnostics_.online = false;
            if (previous_online_) diagnostics_.cable_disconnect_events++;
            previous_online_ = false;
            status_.detail = "SocketCAN receive failed: " + detail;
            return {false, diagnostics_.heartbeat_ok, false};
        }
        diagnostics_.rx_packets++;
        if (!handle_receive_frame(frame, cycle, images.front(), received_tpdo1, received_tpdo2)) {
            diagnostics_.online = false;
            previous_online_ = false;
            status_.detail = "invalid CANopen heartbeat or TPDO frame";
            return {false, diagnostics_.heartbeat_ok, true};
        }
    }

    const bool stale_heartbeat =
        !have_heartbeat_ ||
        cycle - diagnostics_.last_heartbeat_cycle >= config_.heartbeat_timeout_cycles;
    if (stale_heartbeat) {
        if (diagnostics_.heartbeat_ok) diagnostics_.heartbeat_timeout_count++;
        diagnostics_.heartbeat_ok = false;
    }

    const bool pdo_fresh = received_tpdo1 && received_tpdo2;
    if (config_.require_tpdo_each_cycle && !pdo_fresh) {
        diagnostics_.dropout_count++;
        diagnostics_.online = false;
        if (previous_online_) diagnostics_.cable_disconnect_events++;
        previous_online_ = false;
        status_.detail = "configured TPDO set was not received before timeout";
        return {false, diagnostics_.heartbeat_ok, false};
    }

    diagnostics_.online = true;
    previous_online_ = true;
    status_.detail = diagnostics_.heartbeat_ok
                         ? "CANopen TPDO exchange and heartbeat healthy"
                         : "CANopen TPDO exchange healthy; heartbeat not operational or stale";
    return {diagnostics_.heartbeat_ok, diagnostics_.heartbeat_ok, false};
}

const BusDiagnostics& SocketCanCanOpenBackend::diagnostics() const {
    return diagnostics_;
}

AdapterStatus SocketCanCanOpenBackend::status() const {
    return status_;
}

}  // namespace emrtc
