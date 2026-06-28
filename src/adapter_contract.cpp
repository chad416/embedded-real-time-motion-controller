#include "emrtc/adapter_contract.hpp"

#include <algorithm>
#include <sstream>

namespace emrtc {
namespace {

AdapterContractResult fail(FaultCode fault, const std::string& detail) {
    return {false, fault, detail};
}

AdapterObjectMapping to_adapter_mapping(const Cia402ObjectMapping& item) {
    return {item.field,
            item.object_index,
            item.sub_index,
            item.bit_length,
            item.controller_to_drive};
}

}  // namespace

AdapterExchangeMonitor::AdapterExchangeMonitor(std::uint32_t stale_rx_limit_cycles)
    : stale_rx_limit_cycles_(stale_rx_limit_cycles == 0 ? 1 : stale_rx_limit_cycles) {}

AdapterContractResult AdapterExchangeMonitor::observe(CycleIndex cycle,
                                                       const BusExchangeResult& exchange_result,
                                                       const BusDiagnostics& diagnostics) {
    if (exchange_result.invalid_response) {
        return fail(FaultCode::InvalidDriveTransition, "adapter returned invalid PDO/object response");
    }

    if (!diagnostics.online) {
        return fail(FaultCode::BusDropout, "adapter reports bus offline");
    }

    if (!exchange_result.heartbeat_ok || !diagnostics.heartbeat_ok) {
        return fail(FaultCode::HeartbeatTimeout, "adapter heartbeat supervision is not healthy");
    }

    if (!have_rx_sample_ || diagnostics.rx_packets != last_rx_packets_) {
        have_rx_sample_ = true;
        last_rx_cycle_ = cycle;
        last_rx_packets_ = diagnostics.rx_packets;
        return {};
    }

    const auto stale_cycles = cycle >= last_rx_cycle_ ? cycle - last_rx_cycle_ : 0;
    if (stale_cycles >= stale_rx_limit_cycles_) {
        std::ostringstream detail;
        detail << "process image receive data stale for " << stale_cycles << " cycles";
        return fail(FaultCode::BusDropout, detail.str());
    }

    return {};
}

void AdapterExchangeMonitor::reset() {
    have_rx_sample_ = false;
    last_rx_cycle_ = 0;
    last_rx_packets_ = 0;
}

CycleIndex AdapterExchangeMonitor::last_rx_cycle() const {
    return last_rx_cycle_;
}

std::vector<AdapterObjectMapping> expected_adapter_mapping() {
    std::vector<AdapterObjectMapping> result;
    for (const auto& item : cia402_process_image_mapping()) {
        result.push_back(to_adapter_mapping(item));
    }
    return result;
}

AdapterContractResult validate_adapter_mapping(const std::vector<AdapterObjectMapping>& observed) {
    const auto expected = expected_adapter_mapping();
    for (const auto& required : expected) {
        const auto match = std::find_if(observed.begin(), observed.end(), [&](const auto& item) {
            return item.field == required.field;
        });

        if (match == observed.end()) {
            return fail(FaultCode::InvalidDriveTransition,
                        "missing required object mapping for " + required.field);
        }

        if (match->object_index != required.object_index ||
            match->sub_index != required.sub_index ||
            match->bit_length != required.bit_length ||
            match->controller_to_drive != required.controller_to_drive) {
            std::ostringstream detail;
            detail << "invalid object mapping for " << required.field;
            return fail(FaultCode::InvalidDriveTransition, detail.str());
        }
    }

    for (const auto& observed_item : observed) {
        const auto count = std::count_if(observed.begin(), observed.end(), [&](const auto& item) {
            return item.field == observed_item.field;
        });
        if (count > 1) {
            return fail(FaultCode::InvalidDriveTransition,
                        "duplicate object mapping for " + observed_item.field);
        }
    }

    return {};
}

}  // namespace emrtc
