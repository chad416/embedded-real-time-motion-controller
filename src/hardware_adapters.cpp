#include "emrtc/hardware_adapters.hpp"

#include <utility>

namespace emrtc {

PlannedHardwareBackend::PlannedHardwareBackend(std::string backend_name)
    : backend_name_(std::move(backend_name)) {
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
    return {false, backend_name_, "adapter skeleton present; hardware stack not linked or commissioned"};
}

SoemEtherCatBackend::SoemEtherCatBackend()
    : PlannedHardwareBackend("SOEM EtherCAT") {}

EtherLabEtherCatBackend::EtherLabEtherCatBackend()
    : PlannedHardwareBackend("EtherLab EtherCAT") {}

SocketCanCanOpenBackend::SocketCanCanOpenBackend()
    : PlannedHardwareBackend("SocketCAN CANopen") {}

CanOpenNodeBackend::CanOpenNodeBackend()
    : PlannedHardwareBackend("CANopenNode") {}

}  // namespace emrtc
