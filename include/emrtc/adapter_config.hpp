#pragma once

#include "emrtc/adapter_contract.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace emrtc {

struct AdapterStatus {
    bool configured{false};
    bool build_flag_enabled{false};
    bool dependency_available{false};
    std::string backend;
    std::string detail;
};

struct AdapterConfigurationResult {
    bool ok{false};
    std::size_t slave_count{0};
    std::size_t output_bytes{0};
    std::size_t input_bytes{0};
    int expected_working_counter{0};
    std::string detail;
};

struct SoemPdoLayout {
    std::size_t controlword_offset{0};
    std::size_t target_position_offset{2};
    std::size_t target_velocity_offset{6};
    std::size_t statusword_offset{0};
    std::size_t actual_position_offset{2};
    std::size_t actual_velocity_offset{6};
};

struct SoemAdapterConfig {
    std::string interface_name;
    std::uint8_t group{0};
    std::size_t io_map_bytes{4096};
    std::size_t expected_slave_count{1};
    std::uint32_t receive_timeout_us{500};
    double counts_per_unit{1000.0};
    SoemPdoLayout layout;
    std::vector<AdapterObjectMapping> observed_mapping;
};

struct SocketCanCanOpenConfig {
    std::string interface_name;
    std::uint8_t node_id{1};
    std::uint32_t receive_timeout_us{500};
    std::uint32_t heartbeat_timeout_cycles{5};
    double counts_per_unit{1000.0};
    std::uint32_t rpdo1_cob_id{0x201};
    std::uint32_t rpdo2_cob_id{0x301};
    std::uint32_t tpdo1_cob_id{0x181};
    std::uint32_t tpdo2_cob_id{0x281};
    std::uint32_t heartbeat_cob_id{0x701};
    bool require_tpdo_each_cycle{true};
    std::vector<AdapterObjectMapping> observed_mapping;
};

enum class RuntimeAdapterBackend {
    SoemEtherCat,
    SocketCanCanOpen
};

struct RuntimeAdapterConfig {
    std::uint32_t schema_version{1};
    RuntimeAdapterBackend backend{RuntimeAdapterBackend::SoemEtherCat};
    SoemAdapterConfig soem;
    SocketCanCanOpenConfig socketcan;
};

RuntimeAdapterConfig load_runtime_adapter_config(const std::string& path);
void validate_runtime_adapter_config(const RuntimeAdapterConfig& config);
std::string summarize_runtime_adapter_config(const RuntimeAdapterConfig& config);
std::string to_string(RuntimeAdapterBackend backend);

}  // namespace emrtc
