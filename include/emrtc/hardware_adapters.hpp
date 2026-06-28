#pragma once

#include "emrtc/bus.hpp"

#include <string>

namespace emrtc {

struct AdapterStatus {
    bool configured{false};
    std::string backend;
    std::string detail;
};

class PlannedHardwareBackend : public IBusBackend {
public:
    explicit PlannedHardwareBackend(std::string backend_name);

    BusExchangeResult exchange(CycleIndex cycle, std::vector<ProcessImage>& images) override;
    const BusDiagnostics& diagnostics() const override;
    AdapterStatus status() const;

private:
    std::string backend_name_;
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
