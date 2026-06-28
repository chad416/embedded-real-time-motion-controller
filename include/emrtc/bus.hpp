#pragma once

#include "emrtc/types.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace emrtc {

struct ProcessImage {
    std::uint16_t controlword{0};
    std::uint16_t statusword{0};
    double target_position{0.0};
    double target_velocity{0.0};
    double actual_position{0.0};
    double actual_velocity{0.0};
    FaultCode drive_fault{FaultCode::None};
};

struct BusDiagnostics {
    std::uint64_t tx_packets{0};
    std::uint64_t rx_packets{0};
    std::uint64_t packet_loss_count{0};
    std::uint64_t dropout_count{0};
    std::uint64_t invalid_response_count{0};
    std::uint64_t heartbeat_timeout_count{0};
    std::uint64_t cable_disconnect_events{0};
    CycleIndex last_heartbeat_cycle{0};
    bool online{true};
    bool heartbeat_ok{true};
};

struct BusExchangeResult {
    bool ok{true};
    bool heartbeat_ok{true};
    bool invalid_response{false};
};

class IBusBackend {
public:
    virtual ~IBusBackend() = default;
    virtual BusExchangeResult exchange(CycleIndex cycle, std::vector<ProcessImage>& images) = 0;
    virtual const BusDiagnostics& diagnostics() const = 0;
};

class SimulatedBusBackend final : public IBusBackend {
public:
    BusExchangeResult exchange(CycleIndex cycle, std::vector<ProcessImage>& images) override;
    const BusDiagnostics& diagnostics() const override;

    void inject_packet_loss(std::uint32_t every_n_packets, std::uint32_t duration_cycles);
    void inject_dropout(std::uint32_t duration_cycles);
    void inject_heartbeat_timeout(std::uint32_t duration_cycles);
    void inject_invalid_response();
    void reset_faults();

private:
    BusDiagnostics diagnostics_;
    std::uint32_t packet_loss_every_n_{0};
    std::uint32_t packet_loss_cycles_remaining_{0};
    std::uint32_t dropout_cycles_remaining_{0};
    std::uint32_t heartbeat_timeout_cycles_remaining_{0};
    bool invalid_response_pending_{false};
};

}  // namespace emrtc
