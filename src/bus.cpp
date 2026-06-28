#include "emrtc/bus.hpp"

namespace emrtc {

BusExchangeResult SimulatedBusBackend::exchange(CycleIndex cycle, std::vector<ProcessImage>& images) {
    (void)images;
    diagnostics_.tx_packets++;

    BusExchangeResult result;
    result.ok = true;
    result.heartbeat_ok = true;
    result.invalid_response = false;

    if (dropout_cycles_remaining_ > 0) {
        dropout_cycles_remaining_--;
        diagnostics_.dropout_count++;
        diagnostics_.online = false;
        result.ok = false;
        result.heartbeat_ok = false;
        return result;
    }

    diagnostics_.online = true;

    if (packet_loss_cycles_remaining_ > 0 && packet_loss_every_n_ > 0 &&
        diagnostics_.tx_packets % packet_loss_every_n_ == 0) {
        packet_loss_cycles_remaining_--;
        diagnostics_.packet_loss_count++;
        result.ok = false;
        return result;
    }

    if (heartbeat_timeout_cycles_remaining_ > 0) {
        heartbeat_timeout_cycles_remaining_--;
        result.heartbeat_ok = false;
        diagnostics_.heartbeat_ok = false;
        diagnostics_.heartbeat_timeout_count++;
    } else {
        diagnostics_.heartbeat_ok = true;
        diagnostics_.last_heartbeat_cycle = cycle;
    }

    if (invalid_response_pending_) {
        invalid_response_pending_ = false;
        diagnostics_.invalid_response_count++;
        result.invalid_response = true;
        result.ok = false;
        return result;
    }

    diagnostics_.rx_packets++;
    return result;
}

const BusDiagnostics& SimulatedBusBackend::diagnostics() const {
    return diagnostics_;
}

void SimulatedBusBackend::inject_packet_loss(std::uint32_t every_n_packets,
                                             std::uint32_t duration_cycles) {
    packet_loss_every_n_ = every_n_packets == 0 ? 1 : every_n_packets;
    packet_loss_cycles_remaining_ = duration_cycles;
}

void SimulatedBusBackend::inject_dropout(std::uint32_t duration_cycles) {
    dropout_cycles_remaining_ = duration_cycles;
    diagnostics_.cable_disconnect_events++;
}

void SimulatedBusBackend::inject_heartbeat_timeout(std::uint32_t duration_cycles) {
    heartbeat_timeout_cycles_remaining_ = duration_cycles;
}

void SimulatedBusBackend::inject_invalid_response() {
    invalid_response_pending_ = true;
}

void SimulatedBusBackend::reset_faults() {
    packet_loss_every_n_ = 0;
    packet_loss_cycles_remaining_ = 0;
    dropout_cycles_remaining_ = 0;
    heartbeat_timeout_cycles_remaining_ = 0;
    invalid_response_pending_ = false;
    diagnostics_.online = true;
    diagnostics_.heartbeat_ok = true;
}

}  // namespace emrtc
