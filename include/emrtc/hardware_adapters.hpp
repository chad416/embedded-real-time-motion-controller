#pragma once

#include "emrtc/adapter_config.hpp"
#include "emrtc/bus.hpp"
#include "emrtc/canopen.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace emrtc {

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

class CanOpenNodeBackend final : public PlannedHardwareBackend {
public:
    CanOpenNodeBackend();
};

}  // namespace emrtc
