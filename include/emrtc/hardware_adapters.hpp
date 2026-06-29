#pragma once

#include "emrtc/adapter_contract.hpp"
#include "emrtc/bus.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
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

AdapterContractResult encode_soem_process_outputs(const ProcessImage& image,
                                                  double counts_per_unit,
                                                  const SoemPdoLayout& layout,
                                                  std::uint8_t* output_bytes,
                                                  std::size_t output_size);
AdapterContractResult decode_soem_process_inputs(const std::uint8_t* input_bytes,
                                                 std::size_t input_size,
                                                 double counts_per_unit,
                                                 const SoemPdoLayout& layout,
                                                 ProcessImage& image);

class PlannedHardwareBackend : public IBusBackend {
public:
    PlannedHardwareBackend(std::string backend_name, bool build_flag_enabled, bool dependency_available);

    BusExchangeResult exchange(CycleIndex cycle, std::vector<ProcessImage>& images) override;
    const BusDiagnostics& diagnostics() const override;
    AdapterStatus status() const;

private:
    std::string backend_name_;
    bool build_flag_enabled_{false};
    bool dependency_available_{false};
    BusDiagnostics diagnostics_;
};

class SoemEtherCatBackend final : public IBusBackend {
public:
    SoemEtherCatBackend();
    ~SoemEtherCatBackend() override;

    SoemEtherCatBackend(const SoemEtherCatBackend&) = delete;
    SoemEtherCatBackend& operator=(const SoemEtherCatBackend&) = delete;

    AdapterConfigurationResult configure(const SoemAdapterConfig& config);
    void shutdown();
    BusExchangeResult exchange(CycleIndex cycle, std::vector<ProcessImage>& images) override;
    const BusDiagnostics& diagnostics() const override;
    AdapterStatus status() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    SoemAdapterConfig config_;
    BusDiagnostics diagnostics_;
    AdapterStatus status_;
    bool previous_online_{false};
};

class EtherLabEtherCatBackend final : public PlannedHardwareBackend {
public:
    EtherLabEtherCatBackend();
};

class SocketCanCanOpenBackend final : public PlannedHardwareBackend {
public:
    SocketCanCanOpenBackend();
};

class CanOpenNodeBackend final : public PlannedHardwareBackend {
public:
    CanOpenNodeBackend();
};

}  // namespace emrtc
