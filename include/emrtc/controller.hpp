#pragma once

#include "emrtc/axis.hpp"
#include "emrtc/bus.hpp"
#include "emrtc/fault_injector.hpp"
#include "emrtc/telemetry.hpp"
#include "emrtc/watchdog.hpp"

#include <string>
#include <vector>

namespace emrtc {

class RealTimeController {
public:
    explicit RealTimeController(ControllerConfig config = {});
    RealTimeController(IBusBackend& backend, ControllerConfig config = {});

    Axis& axis();
    const Axis& axis() const;
    SimulatedBusBackend& bus();
    const SimulatedBusBackend& bus() const;
    IBusBackend& backend();
    const IBusBackend& backend() const;
    bool using_simulated_backend() const;
    FaultInjector& fault_injector();

    void command(AxisCommand command);
    void request_estop();
    void reset_faults();
    void cold_restart();
    void warm_restart();
    void run_cycles(std::uint64_t count);
    RuntimeMetrics metrics() const;
    ControllerConfig config() const;

private:
    Nanoseconds deterministic_period_for_cycle(CycleIndex cycle) const;
    void apply_fault_event(const FaultInjectionEvent& event);
    void execute_cycle();
    void record_status(const std::string& event);
    void record_fault(FaultCode fault, const std::string& detail);

    ControllerConfig config_;
    Axis axis_;
    SimulatedBusBackend simulated_bus_;
    IBusBackend* backend_{nullptr};
    FaultInjector fault_injector_;
    WatchdogSupervisor watchdog_;
    TelemetryRecorder telemetry_;
    std::vector<ProcessImage> process_images_;
    AxisCommand pending_command_;
    CycleIndex cycle_{0};
    bool estop_requested_{false};
    bool force_watchdog_timeout_{false};
    Nanoseconds forced_extra_period_ns_{0};
    FaultCode last_recorded_fault_{FaultCode::None};
};

std::vector<ScenarioSummary> run_validation_scenarios(RuntimeMetrics* combined_metrics = nullptr);
RuntimeMetrics run_benchmark(std::uint64_t cycles);
RecoveryPerformanceMetrics run_fault_recovery_benchmark();

}  // namespace emrtc
