#pragma once

#include "emrtc/bus.hpp"

#include <string>

namespace emrtc {

struct AdapterStatus {
    bool configured{false};
    bool build_flag_enabled{false};
    bool dependency_available{false};
    std::string backend;
    std::string detail;
};

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

class SoemEtherCatBackend final : public PlannedHardwareBackend {
public:
    SoemEtherCatBackend();
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
