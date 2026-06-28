#include "emrtc/hardware_adapters.hpp"

#include <utility>

namespace emrtc {

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

SoemEtherCatBackend::SoemEtherCatBackend()
    : PlannedHardwareBackend("SOEM EtherCAT",
#ifdef EMRTC_ENABLE_SOEM
                             true,
#else
                             false,
#endif
#ifdef EMRTC_HAS_SOEM
                             true
#else
                             false
#endif
      ) {}

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
