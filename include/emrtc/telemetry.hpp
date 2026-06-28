#pragma once

#include "emrtc/bus.hpp"
#include "emrtc/types.hpp"

#include <string>
#include <vector>

namespace emrtc {

struct TraceEvent {
    CycleIndex cycle{0};
    Nanoseconds timestamp_ns{0};
    std::string event;
    DriveState state{DriveState::NotReady};
    ControlMode mode{ControlMode::Disabled};
    FaultCode fault{FaultCode::None};
    double position{0.0};
    double velocity{0.0};
    double following_error{0.0};
};

struct FaultLogEntry {
    CycleIndex cycle{0};
    Nanoseconds timestamp_ns{0};
    FaultCode fault{FaultCode::None};
    std::string detail;
};

struct RuntimeMetrics {
    std::vector<CycleSample> cycles;
    std::vector<TraceEvent> trace;
    std::vector<FaultLogEntry> fault_log;
    std::vector<ScenarioSummary> scenarios;
    BusDiagnostics bus;
    Nanoseconds min_cycle_ns{0};
    Nanoseconds max_cycle_ns{0};
    double mean_cycle_ns{0.0};
    Nanoseconds min_jitter_ns{0};
    Nanoseconds max_jitter_ns{0};
    double mean_abs_jitter_ns{0.0};
    std::uint64_t missed_cycles{0};
    std::uint64_t deadline_overruns{0};
};

class TelemetryRecorder {
public:
    void record_cycle(CycleSample sample);
    void record_trace(TraceEvent event);
    void record_fault(FaultLogEntry fault);
    void record_scenario(ScenarioSummary scenario);
    void set_bus_diagnostics(BusDiagnostics diagnostics);
    RuntimeMetrics finalize() const;
    void clear();

    static bool write_trace_csv(const RuntimeMetrics& metrics, const std::string& path);
    static bool write_metrics_json(const RuntimeMetrics& metrics, const std::string& path);
    static bool write_fault_log_json(const RuntimeMetrics& metrics, const std::string& path);
    static bool write_validation_markdown(const RuntimeMetrics& metrics, const std::string& path);

private:
    std::vector<CycleSample> cycles_;
    std::vector<TraceEvent> trace_;
    std::vector<FaultLogEntry> fault_log_;
    std::vector<ScenarioSummary> scenarios_;
    BusDiagnostics bus_;
};

}  // namespace emrtc
