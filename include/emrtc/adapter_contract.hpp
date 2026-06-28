#pragma once

#include "emrtc/bus.hpp"
#include "emrtc/process_image_mapping.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace emrtc {

struct AdapterObjectMapping {
    std::string field;
    std::uint16_t object_index{0};
    std::uint8_t sub_index{0};
    std::uint8_t bit_length{0};
    bool controller_to_drive{false};
};

struct AdapterContractResult {
    bool ok{true};
    FaultCode fault{FaultCode::None};
    std::string detail{"ok"};
};

class AdapterExchangeMonitor {
public:
    explicit AdapterExchangeMonitor(std::uint32_t stale_rx_limit_cycles = 5);

    AdapterContractResult observe(CycleIndex cycle,
                                  const BusExchangeResult& exchange_result,
                                  const BusDiagnostics& diagnostics);
    void reset();
    CycleIndex last_rx_cycle() const;

private:
    std::uint32_t stale_rx_limit_cycles_{5};
    bool have_rx_sample_{false};
    CycleIndex last_rx_cycle_{0};
    std::uint64_t last_rx_packets_{0};
};

std::vector<AdapterObjectMapping> expected_adapter_mapping();
AdapterContractResult validate_adapter_mapping(const std::vector<AdapterObjectMapping>& observed);

}  // namespace emrtc
