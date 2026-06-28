#include "emrtc/hardware_adapters.hpp"

#include <utility>

namespace emrtc {

PlannedHardwareBackend::PlannedHardwareBackend(std::string backend_name, bool build_flag_enabled)
    : backend_name_(std::move(backend_name)), build_flag_enabled_(build_flag_enabled) {
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
    const std::string detail = build_flag_enabled_
                                   ? "adapter build flag enabled; real hardware stack still not linked or commissioned"
                                   : "adapter skeleton present; enable build flag and link hardware stack before commissioning";
    return {false, build_flag_enabled_, backend_name_, detail};
}

SoemEtherCatBackend::SoemEtherCatBackend()
    : PlannedHardwareBackend("SOEM EtherCAT",
#ifdef EMRTC_ENABLE_SOEM
                             true
#else
                             false
#endif
      ) {}

EtherLabEtherCatBackend::EtherLabEtherCatBackend()
    : PlannedHardwareBackend("EtherLab EtherCAT",
#ifdef EMRTC_ENABLE_ETHERLAB
                             true
#else
                             false
#endif
      ) {}

SocketCanCanOpenBackend::SocketCanCanOpenBackend()
    : PlannedHardwareBackend("SocketCAN CANopen",
#ifdef EMRTC_ENABLE_SOCKETCAN
                             true
#else
                             false
#endif
      ) {}

CanOpenNodeBackend::CanOpenNodeBackend()
    : PlannedHardwareBackend("CANopenNode",
#ifdef EMRTC_ENABLE_CANOPENNODE
                             true
#else
                             false
#endif
      ) {}

}  // namespace emrtc
