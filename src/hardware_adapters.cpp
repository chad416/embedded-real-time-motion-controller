#include "emrtc/hardware_adapters.hpp"

#include "emrtc/process_image_mapping.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>
#include <utility>

#ifdef EMRTC_HAS_SOEM
extern "C" {
#include <soem/soem.h>
}
#endif

namespace emrtc {
namespace {

AdapterConfigurationResult configuration_failure(const std::string& detail) {
    return {false, 0, 0, 0, 0, detail};
}

bool range_fits(std::size_t offset, std::size_t width, std::size_t available) {
    return offset <= available && width <= available - offset;
}

void write_u16(std::uint8_t* bytes, std::size_t offset, std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

void write_i32(std::uint8_t* bytes, std::size_t offset, std::int32_t value) {
    const auto encoded = static_cast<std::uint32_t>(value);
    for (std::size_t index = 0; index < 4; ++index) {
        bytes[offset + index] =
            static_cast<std::uint8_t>((encoded >> (index * 8U)) & 0xFFU);
    }
}

std::uint16_t read_u16(const std::uint8_t* bytes, std::size_t offset) {
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(bytes[offset]) |
        (static_cast<std::uint16_t>(bytes[offset + 1]) << 8U));
}

std::int32_t read_i32(const std::uint8_t* bytes, std::size_t offset) {
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < 4; ++index) {
        value |= static_cast<std::uint32_t>(bytes[offset + index]) << (index * 8U);
    }
    return static_cast<std::int32_t>(value);
}

bool output_layout_fits(const SoemPdoLayout& layout, std::size_t bytes) {
    return range_fits(layout.controlword_offset, 2, bytes) &&
           range_fits(layout.target_position_offset, 4, bytes) &&
           range_fits(layout.target_velocity_offset, 4, bytes);
}

bool input_layout_fits(const SoemPdoLayout& layout, std::size_t bytes) {
    return range_fits(layout.statusword_offset, 2, bytes) &&
           range_fits(layout.actual_position_offset, 4, bytes) &&
           range_fits(layout.actual_velocity_offset, 4, bytes);
}

}  // namespace

AdapterContractResult encode_soem_process_outputs(const ProcessImage& image,
                                                  double counts_per_unit,
                                                  const SoemPdoLayout& layout,
                                                  std::uint8_t* output_bytes,
                                                  std::size_t output_size) {
    if (output_bytes == nullptr || !output_layout_fits(layout, output_size)) {
        return {false,
                FaultCode::InvalidDriveTransition,
                "SOEM output PDO layout exceeds mapped process data"};
    }
    const auto wire = encode_cia402_wire_image(image, counts_per_unit);
    write_u16(output_bytes, layout.controlword_offset, wire.controlword);
    write_i32(output_bytes, layout.target_position_offset, wire.target_position_counts);
    write_i32(output_bytes, layout.target_velocity_offset, wire.target_velocity_counts);
    return {};
}

AdapterContractResult decode_soem_process_inputs(const std::uint8_t* input_bytes,
                                                 std::size_t input_size,
                                                 double counts_per_unit,
                                                 const SoemPdoLayout& layout,
                                                 ProcessImage& image) {
    if (input_bytes == nullptr || !input_layout_fits(layout, input_size)) {
        return {false,
                FaultCode::InvalidDriveTransition,
                "SOEM input PDO layout exceeds mapped process data"};
    }
    auto wire = encode_cia402_wire_image(image, counts_per_unit);
    wire.statusword = read_u16(input_bytes, layout.statusword_offset);
    wire.actual_position_counts = read_i32(input_bytes, layout.actual_position_offset);
    wire.actual_velocity_counts = read_i32(input_bytes, layout.actual_velocity_offset);
    wire.drive_fault =
        (wire.statusword & 0x0008U) != 0U ? FaultCode::DriveFault : FaultCode::None;
    image = decode_cia402_wire_image(wire, counts_per_unit);
    return {};
}

PlannedHardwareBackend::PlannedHardwareBackend(std::string backend_name,
                                               bool build_flag_enabled,
                                               bool dependency_available)
    : backend_name_(std::move(backend_name)),
      build_flag_enabled_(build_flag_enabled),
      dependency_available_(dependency_available) {
    diagnostics_.online = false;
    diagnostics_.heartbeat_ok = false;
}

BusExchangeResult PlannedHardwareBackend::exchange(CycleIndex cycle, std::vector<ProcessImage>& images) {
    (void)cycle;
    (void)images;
    diagnostics_.tx_packets++;
    diagnostics_.dropout_count++;
    return {false, false, false};
}

const BusDiagnostics& PlannedHardwareBackend::diagnostics() const {
    return diagnostics_;
}

AdapterStatus PlannedHardwareBackend::status() const {
    std::string detail;
    if (!build_flag_enabled_) {
        detail = "adapter skeleton present; enable build flag and link hardware stack before commissioning";
    } else if (dependency_available_) {
        detail = "adapter build flag enabled and dependency discovered; process-image mapping still not commissioned";
    } else {
        detail = "adapter build flag enabled; external dependency not discovered, using skeleton mode";
    }
    return {false, build_flag_enabled_, dependency_available_, backend_name_, detail};
}

struct SoemEtherCatBackend::Impl {
#ifdef EMRTC_ENABLE_SOEM
    static constexpr bool build_flag_enabled = true;
#else
    static constexpr bool build_flag_enabled = false;
#endif
#ifdef EMRTC_HAS_SOEM
    static constexpr bool dependency_available = true;
    ecx_contextt context{};
    std::vector<std::uint8_t> io_map;
    bool socket_open{false};
    int expected_working_counter{0};
#else
    static constexpr bool dependency_available = false;
#endif
};

SoemEtherCatBackend::SoemEtherCatBackend()
    : impl_(std::make_unique<Impl>()) {
    diagnostics_.online = false;
    diagnostics_.heartbeat_ok = false;
    status_ = {false,
               Impl::build_flag_enabled,
               Impl::dependency_available,
               "SOEM EtherCAT",
               Impl::dependency_available
                   ? "SOEM v2 dependency available; adapter requires explicit configuration"
                   : "SOEM v2 dependency unavailable; adapter remains in skeleton mode"};
}

SoemEtherCatBackend::~SoemEtherCatBackend() {
    shutdown();
}

AdapterConfigurationResult SoemEtherCatBackend::configure(const SoemAdapterConfig& config) {
    shutdown();
    const auto fail = [this](const std::string& detail) {
        status_.configured = false;
        status_.detail = detail;
        return configuration_failure(detail);
    };
    if (config.interface_name.empty()) {
        return fail("SOEM interface name must not be empty");
    }
    if (config.io_map_bytes < 4096U) {
        return fail("SOEM I/O map must reserve at least 4096 bytes");
    }
    if (config.expected_slave_count != 1U) {
        return fail("SOEM single-axis backend requires expected_slave_count=1");
    }
    if (config.receive_timeout_us == 0U ||
        config.receive_timeout_us >
            static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        return fail("SOEM receive_timeout_us must fit a positive int");
    }
    if (config.counts_per_unit <= 0.0) {
        return fail("SOEM counts_per_unit must be positive");
    }

    const auto mapping_result = validate_adapter_mapping(config.observed_mapping);
    if (!mapping_result.ok) {
        return fail(mapping_result.detail);
    }
    if (!Impl::build_flag_enabled) {
        return fail("SOEM build flag is disabled");
    }
    if (!Impl::dependency_available) {
        return fail("SOEM v2 dependency is unavailable");
    }

    config_ = config;
#ifdef EMRTC_HAS_SOEM
    if (config.group >= EC_MAXGROUP) {
        return fail("SOEM group index exceeds EC_MAXGROUP");
    }
    if (!output_layout_fits(config.layout, config.io_map_bytes) ||
        !input_layout_fits(config.layout, config.io_map_bytes)) {
        return fail("SOEM PDO byte layout exceeds reserved I/O map");
    }
    std::memset(&impl_->context, 0, sizeof(impl_->context));
    impl_->io_map.assign(config.io_map_bytes, 0U);
    if (!ecx_init(&impl_->context, config_.interface_name.data())) {
        return fail("SOEM could not open the selected interface");
    }
    impl_->socket_open = true;

    const int slave_count = ecx_config_init(&impl_->context);
    if (slave_count <= 0) {
        shutdown();
        return fail("SOEM found no EtherCAT slaves");
    }
    if (static_cast<std::size_t>(slave_count) != config.expected_slave_count) {
        std::ostringstream detail;
        detail << "SOEM expected " << config.expected_slave_count
               << " slaves but found " << slave_count;
        shutdown();
        return fail(detail.str());
    }

    const int mapped_bytes =
        ecx_config_map_group(&impl_->context, impl_->io_map.data(), config.group);
    if (mapped_bytes <= 0 ||
        static_cast<std::size_t>(mapped_bytes) > impl_->io_map.size()) {
        shutdown();
        return fail("SOEM mapped process image exceeds reserved I/O map");
    }
    auto* group = impl_->context.grouplist + config.group;
    const auto output_bytes = static_cast<std::size_t>(group->Obytes);
    const auto input_bytes = static_cast<std::size_t>(group->Ibytes);
    if (group->outputs == nullptr ||
        group->inputs == nullptr ||
        !output_layout_fits(config.layout, output_bytes) ||
        !input_layout_fits(config.layout, input_bytes)) {
        shutdown();
        return fail("configured SOEM PDO byte layout exceeds mapped I/O");
    }

    impl_->expected_working_counter = group->outputsWKC * 2 + group->inputsWKC;
    if (impl_->expected_working_counter <= 0) {
        shutdown();
        return fail("SOEM mapped group has no valid process-data working counter");
    }

    ecx_configdc(&impl_->context);
    ecx_statecheck(&impl_->context,
                   0,
                   EC_STATE_SAFE_OP,
                   EC_TIMEOUTSTATE * 4);
    ecx_send_processdata(&impl_->context);
    ecx_receive_processdata(&impl_->context,
                            static_cast<int>(config.receive_timeout_us));

    auto* broadcast_slave = impl_->context.slavelist;
    broadcast_slave->state = EC_STATE_OPERATIONAL;
    ecx_writestate(&impl_->context, 0);

    bool operational = false;
    for (int attempt = 0; attempt < 10; ++attempt) {
        ecx_send_processdata(&impl_->context);
        ecx_receive_processdata(&impl_->context,
                                static_cast<int>(config.receive_timeout_us));
        ecx_statecheck(&impl_->context,
                       0,
                       EC_STATE_OPERATIONAL,
                       EC_TIMEOUTSTATE / 10);
        if (broadcast_slave->state == EC_STATE_OPERATIONAL) {
            operational = true;
            break;
        }
    }
    if (!operational) {
        shutdown();
        return fail("SOEM slaves did not reach OPERATIONAL state");
    }

    diagnostics_ = {};
    diagnostics_.online = true;
    diagnostics_.heartbeat_ok = true;
    previous_online_ = true;
    status_.configured = true;
    status_.detail = "SOEM v2 configured and all mapped slaves reached OPERATIONAL";
    return {true,
            static_cast<std::size_t>(slave_count),
            output_bytes,
            input_bytes,
            impl_->expected_working_counter,
            status_.detail};
#else
    return fail("SOEM v2 dependency is unavailable");
#endif
}

void SoemEtherCatBackend::shutdown() {
#ifdef EMRTC_HAS_SOEM
    if (impl_ && impl_->socket_open) {
        if (impl_->context.slavecount > 0) {
            impl_->context.slavelist->state = EC_STATE_INIT;
            ecx_writestate(&impl_->context, 0);
        }
        ecx_close(&impl_->context);
        std::memset(&impl_->context, 0, sizeof(impl_->context));
        impl_->io_map.clear();
        impl_->socket_open = false;
        impl_->expected_working_counter = 0;
    }
#endif
    diagnostics_ = {};
    diagnostics_.online = false;
    diagnostics_.heartbeat_ok = false;
    previous_online_ = false;
    if (impl_) {
        status_.configured = false;
        status_.detail = Impl::dependency_available
                             ? "SOEM v2 dependency available; adapter requires explicit configuration"
                             : "SOEM v2 dependency unavailable; adapter remains in skeleton mode";
    }
}

BusExchangeResult SoemEtherCatBackend::exchange(CycleIndex cycle,
                                                std::vector<ProcessImage>& images) {
    diagnostics_.tx_packets++;
    if (!status_.configured || images.size() != 1U) {
        diagnostics_.dropout_count++;
        diagnostics_.online = false;
        diagnostics_.heartbeat_ok = false;
        const bool invalid_response = images.size() != 1U;
        if (invalid_response) {
            diagnostics_.invalid_response_count++;
        }
        return {false, false, invalid_response};
    }

#ifdef EMRTC_HAS_SOEM
    auto* group = impl_->context.grouplist + config_.group;
    auto* outputs = group->outputs;
    const auto encode_result =
        encode_soem_process_outputs(images.front(),
                                    config_.counts_per_unit,
                                    config_.layout,
                                    outputs,
                                    static_cast<std::size_t>(group->Obytes));
    if (!encode_result.ok) {
        diagnostics_.invalid_response_count++;
        return {false, diagnostics_.heartbeat_ok, true};
    }

    ecx_send_processdata(&impl_->context);
    const int working_counter =
        ecx_receive_processdata(&impl_->context,
                                static_cast<int>(config_.receive_timeout_us));
    if (working_counter < impl_->expected_working_counter) {
        diagnostics_.dropout_count++;
        if (previous_online_) {
            diagnostics_.cable_disconnect_events++;
        }
        diagnostics_.online = false;
        diagnostics_.heartbeat_ok = false;
        previous_online_ = false;
        return {false, false, false};
    }

    previous_online_ = true;
    diagnostics_.online = true;
    diagnostics_.heartbeat_ok = true;
    diagnostics_.last_heartbeat_cycle = cycle;
    diagnostics_.rx_packets++;

    const auto decode_result =
        decode_soem_process_inputs(group->inputs,
                                   static_cast<std::size_t>(group->Ibytes),
                                   config_.counts_per_unit,
                                   config_.layout,
                                   images.front());
    if (!decode_result.ok) {
        diagnostics_.invalid_response_count++;
        return {false, true, true};
    }
    return {};
#else
    (void)cycle;
    return {false, false, false};
#endif
}

const BusDiagnostics& SoemEtherCatBackend::diagnostics() const {
    return diagnostics_;
}

AdapterStatus SoemEtherCatBackend::status() const {
    return status_;
}

EtherLabEtherCatBackend::EtherLabEtherCatBackend()
    : PlannedHardwareBackend("EtherLab EtherCAT",
#ifdef EMRTC_ENABLE_ETHERLAB
                             true,
#else
                             false,
#endif
#ifdef EMRTC_HAS_ETHERLAB
                             true
#else
                             false
#endif
      ) {}

SocketCanCanOpenBackend::SocketCanCanOpenBackend()
    : PlannedHardwareBackend("SocketCAN CANopen",
#ifdef EMRTC_ENABLE_SOCKETCAN
                             true,
#else
                             false,
#endif
#ifdef EMRTC_HAS_SOCKETCAN
                             true
#else
                             false
#endif
      ) {}

CanOpenNodeBackend::CanOpenNodeBackend()
    : PlannedHardwareBackend("CANopenNode",
#ifdef EMRTC_ENABLE_CANOPENNODE
                             true,
#else
                             false,
#endif
#ifdef EMRTC_HAS_CANOPENNODE
                             true
#else
                             false
#endif
      ) {}

}  // namespace emrtc
