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
    double target_position{0.0};
    double target_velocity{0.0};
    double following_error{0.0};
};

struct FaultLogEntry {
    CycleIndex cycle{0};
    Nanoseconds timestamp_ns{0};
    FaultCode fault{FaultCode::None};
    std::string detail;
};

struct MotionPerformanceMetrics {
    bool available{false};
    bool settled{false};
    double target_position{0.0};
    double final_position{0.0};
    double final_velocity{0.0};
    double overshoot{0.0};
    double settling_time_s{0.0};
    double steady_state_error{0.0};
    double max_abs_following_error{0.0};
};

struct TwoAxisPerformanceMetrics {
    bool available{false};
    bool target_reached{false};
    bool path_within_limit{false};
    bool acceptance_pass{false};
    std::uint64_t cycle_count{0};
    double target_x{0.0};
    double target_y{0.0};
    double final_x{0.0};
    double final_y{0.0};
    double final_x_error{0.0};
    double final_y_error{0.0};
    double final_position_error{0.0};
    double rms_path_error{0.0};
    double max_path_error{0.0};
    double path_error_limit{0.0};
    double settling_time_s{0.0};
    double settling_time_limit_s{0.0};
    double target_tolerance{0.0};
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
    MotionPerformanceMetrics motion;
    TwoAxisPerformanceMetrics two_axis;
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
