#include "emrtc/telemetry.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

namespace emrtc {
namespace {

std::string json_escape(const std::string& value) {
    std::ostringstream out;
    for (const char c : value) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c; break;
        }
    }
    return out.str();
}

template <typename T>
void write_json_field(std::ostream& out, const std::string& name, T value, bool comma = true) {
    out << "  \"" << name << "\": " << value;
    if (comma) {
        out << ",";
    }
    out << "\n";
}

void write_json_bool_field(std::ostream& out, const std::string& name, bool value, bool comma = true) {
    out << "  \"" << name << "\": " << (value ? "true" : "false");
    if (comma) {
        out << ",";
    }
    out << "\n";
}

MotionPerformanceMetrics compute_motion_performance(const std::vector<TraceEvent>& trace) {
    MotionPerformanceMetrics result;
    if (trace.empty()) {
        return result;
    }

    result.available = true;
    result.target_position = trace.back().target_position;
    result.final_position = trace.back().position;
    result.final_velocity = trace.back().velocity;
    result.steady_state_error = result.target_position - result.final_position;

    const auto initial_position = trace.front().position;
    double max_position = trace.front().position;
    double min_position = trace.front().position;
    for (const auto& event : trace) {
        max_position = std::max(max_position, event.position);
        min_position = std::min(min_position, event.position);
        result.max_abs_following_error =
            std::max(result.max_abs_following_error, std::abs(event.following_error));
    }

    if (result.target_position >= initial_position) {
        result.overshoot = std::max(0.0, max_position - result.target_position);
    } else {
        result.overshoot = std::max(0.0, result.target_position - min_position);
    }

    constexpr double kPositionTolerance = 0.010;
    constexpr double kVelocityTolerance = 0.010;
    std::vector<bool> suffix_ok(trace.size(), false);
    bool all_ok = true;
    for (std::size_t offset = 0; offset < trace.size(); ++offset) {
        const auto index = trace.size() - 1 - offset;
        const auto& event = trace[index];
        const bool current_ok =
            std::abs(event.position - result.target_position) <= kPositionTolerance &&
            std::abs(event.velocity) <= kVelocityTolerance &&
            event.fault == FaultCode::None;
        all_ok = all_ok && current_ok;
        suffix_ok[index] = all_ok;
    }

    for (std::size_t i = 0; i < suffix_ok.size(); ++i) {
        if (suffix_ok[i]) {
            result.settled = true;
            result.settling_time_s = static_cast<double>(trace[i].timestamp_ns) * 1.0e-9;
            break;
        }
    }

    return result;
}

}  // namespace

void TelemetryRecorder::record_cycle(CycleSample sample) {
    cycles_.push_back(sample);
}

void TelemetryRecorder::record_trace(TraceEvent event) {
    trace_.push_back(std::move(event));
}

void TelemetryRecorder::record_fault(FaultLogEntry fault) {
    fault_log_.push_back(std::move(fault));
}

void TelemetryRecorder::record_scenario(ScenarioSummary scenario) {
    scenarios_.push_back(std::move(scenario));
}

void TelemetryRecorder::set_bus_diagnostics(BusDiagnostics diagnostics) {
    bus_ = diagnostics;
}

RuntimeMetrics TelemetryRecorder::finalize() const {
    RuntimeMetrics metrics;
    metrics.cycles = cycles_;
    metrics.trace = trace_;
    metrics.fault_log = fault_log_;
    metrics.scenarios = scenarios_;
    metrics.bus = bus_;

    if (cycles_.empty()) {
        return metrics;
    }

    metrics.min_cycle_ns = cycles_.front().measured_period_ns;
    metrics.max_cycle_ns = cycles_.front().measured_period_ns;
    metrics.min_jitter_ns = cycles_.front().jitter_ns;
    metrics.max_jitter_ns = cycles_.front().jitter_ns;

    long double total_cycle = 0.0;
    long double total_abs_jitter = 0.0;
    for (const auto& sample : cycles_) {
        metrics.min_cycle_ns = std::min(metrics.min_cycle_ns, sample.measured_period_ns);
        metrics.max_cycle_ns = std::max(metrics.max_cycle_ns, sample.measured_period_ns);
        metrics.min_jitter_ns = std::min(metrics.min_jitter_ns, sample.jitter_ns);
        metrics.max_jitter_ns = std::max(metrics.max_jitter_ns, sample.jitter_ns);
        total_cycle += static_cast<long double>(sample.measured_period_ns);
        total_abs_jitter += std::abs(static_cast<long double>(sample.jitter_ns));
        if (sample.missed_cycle) {
            metrics.missed_cycles++;
        }
        if (sample.deadline_overrun) {
            metrics.deadline_overruns++;
        }
    }

    metrics.mean_cycle_ns = static_cast<double>(total_cycle / cycles_.size());
    metrics.mean_abs_jitter_ns = static_cast<double>(total_abs_jitter / cycles_.size());
    metrics.motion = compute_motion_performance(trace_);
    return metrics;
}

void TelemetryRecorder::clear() {
    cycles_.clear();
    trace_.clear();
    fault_log_.clear();
    scenarios_.clear();
    bus_ = {};
}

bool TelemetryRecorder::write_trace_csv(const RuntimeMetrics& metrics, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        return false;
    }

    out << "cycle,timestamp_ns,event,state,mode,fault,position,velocity,target_position,target_velocity,following_error\n";
    out << std::fixed << std::setprecision(6);
    for (const auto& event : metrics.trace) {
        out << event.cycle << ','
            << event.timestamp_ns << ','
            << '"' << event.event << '"' << ','
            << '"' << to_string(event.state) << '"' << ','
            << '"' << to_string(event.mode) << '"' << ','
            << '"' << to_string(event.fault) << '"' << ','
            << event.position << ','
            << event.velocity << ','
            << event.target_position << ','
            << event.target_velocity << ','
            << event.following_error << '\n';
    }
    return true;
}

bool TelemetryRecorder::write_metrics_json(const RuntimeMetrics& metrics, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        return false;
    }

    out << "{\n";
    write_json_field(out, "cycle_count", metrics.cycles.size());
    write_json_field(out, "min_cycle_ns", metrics.min_cycle_ns);
    write_json_field(out, "max_cycle_ns", metrics.max_cycle_ns);
    write_json_field(out, "mean_cycle_ns", metrics.mean_cycle_ns);
    write_json_field(out, "min_jitter_ns", metrics.min_jitter_ns);
    write_json_field(out, "max_jitter_ns", metrics.max_jitter_ns);
    write_json_field(out, "mean_abs_jitter_ns", metrics.mean_abs_jitter_ns);
    write_json_field(out, "missed_cycles", metrics.missed_cycles);
    write_json_field(out, "deadline_overruns", metrics.deadline_overruns);
    out << "  \"motion_performance\": {\n";
    write_json_bool_field(out, "available", metrics.motion.available);
    write_json_bool_field(out, "settled", metrics.motion.settled);
    write_json_field(out, "target_position", metrics.motion.target_position);
    write_json_field(out, "final_position", metrics.motion.final_position);
    write_json_field(out, "final_velocity", metrics.motion.final_velocity);
    write_json_field(out, "overshoot", metrics.motion.overshoot);
    write_json_field(out, "settling_time_s", metrics.motion.settling_time_s);
    write_json_field(out, "steady_state_error", metrics.motion.steady_state_error);
    write_json_field(out, "max_abs_following_error", metrics.motion.max_abs_following_error, false);
    out << "  },\n";
    out << "  \"two_axis_performance\": {\n";
    write_json_bool_field(out, "available", metrics.two_axis.available);
    write_json_bool_field(out, "target_reached", metrics.two_axis.target_reached);
    write_json_bool_field(out, "path_within_limit", metrics.two_axis.path_within_limit);
    write_json_bool_field(out, "acceptance_pass", metrics.two_axis.acceptance_pass);
    write_json_field(out, "cycle_count", metrics.two_axis.cycle_count);
    write_json_field(out, "target_x", metrics.two_axis.target_x);
    write_json_field(out, "target_y", metrics.two_axis.target_y);
    write_json_field(out, "final_x", metrics.two_axis.final_x);
    write_json_field(out, "final_y", metrics.two_axis.final_y);
    write_json_field(out, "final_x_error", metrics.two_axis.final_x_error);
    write_json_field(out, "final_y_error", metrics.two_axis.final_y_error);
    write_json_field(out, "final_position_error", metrics.two_axis.final_position_error);
    write_json_field(out, "rms_path_error", metrics.two_axis.rms_path_error);
    write_json_field(out, "max_path_error", metrics.two_axis.max_path_error);
    write_json_field(out, "path_error_limit", metrics.two_axis.path_error_limit);
    write_json_field(out, "settling_time_s", metrics.two_axis.settling_time_s);
    write_json_field(out, "settling_time_limit_s", metrics.two_axis.settling_time_limit_s);
    write_json_field(out, "target_tolerance", metrics.two_axis.target_tolerance, false);
    out << "  },\n";
    out << "  \"recovery_performance\": {\n";
    write_json_bool_field(out, "available", metrics.recovery.available);
    write_json_bool_field(out, "fault_observed", metrics.recovery.fault_observed);
    write_json_bool_field(out, "operation_reenabled", metrics.recovery.operation_reenabled);
    write_json_bool_field(out, "post_recovery_motion_settled", metrics.recovery.post_recovery_motion_settled);
    write_json_field(out, "fault_injection_cycle", metrics.recovery.fault_injection_cycle);
    write_json_field(out, "fault_detection_cycle", metrics.recovery.fault_detection_cycle);
    write_json_field(out, "operation_reenabled_cycle", metrics.recovery.operation_reenabled_cycle);
    write_json_field(out, "detection_latency_cycles", metrics.recovery.detection_latency_cycles);
    write_json_field(out, "recovery_cycles", metrics.recovery.recovery_cycles);
    write_json_field(out, "baseline_settling_time_s", metrics.recovery.baseline_settling_time_s);
    write_json_field(out, "post_recovery_settling_time_s", metrics.recovery.post_recovery_settling_time_s);
    write_json_field(out, "settling_time_ratio", metrics.recovery.settling_time_ratio);
    write_json_field(out, "post_recovery_final_error", metrics.recovery.post_recovery_final_error, false);
    out << "  },\n";
    out << "  \"bus\": {\n";
    write_json_field(out, "tx_packets", metrics.bus.tx_packets);
    write_json_field(out, "rx_packets", metrics.bus.rx_packets);
    write_json_field(out, "packet_loss_count", metrics.bus.packet_loss_count);
    write_json_field(out, "dropout_count", metrics.bus.dropout_count);
    write_json_field(out, "invalid_response_count", metrics.bus.invalid_response_count);
    write_json_field(out, "heartbeat_timeout_count", metrics.bus.heartbeat_timeout_count);
    write_json_field(out, "cable_disconnect_events", metrics.bus.cable_disconnect_events, false);
    out << "  },\n";
    out << "  \"scenarios\": [\n";
    for (std::size_t i = 0; i < metrics.scenarios.size(); ++i) {
        const auto& scenario = metrics.scenarios[i];
        out << "    {\"name\":\"" << json_escape(scenario.name)
            << "\",\"result\":\"" << to_string(scenario.result)
            << "\",\"evidence\":\"" << json_escape(scenario.evidence)
            << "\",\"recovery_cycles\":" << scenario.recovery_cycles << "}";
        if (i + 1 != metrics.scenarios.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ]\n";
    out << "}\n";
    return true;
}

bool TelemetryRecorder::write_fault_log_json(const RuntimeMetrics& metrics, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        return false;
    }

    out << "[\n";
    for (std::size_t i = 0; i < metrics.fault_log.size(); ++i) {
        const auto& entry = metrics.fault_log[i];
        out << "  {\"cycle\":" << entry.cycle
            << ",\"timestamp_ns\":" << entry.timestamp_ns
            << ",\"fault\":\"" << to_string(entry.fault)
            << "\",\"detail\":\"" << json_escape(entry.detail) << "\"}";
        if (i + 1 != metrics.fault_log.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "]\n";
    return true;
}

bool TelemetryRecorder::write_validation_markdown(const RuntimeMetrics& metrics, const std::string& path) {
    std::ofstream out(path);
    if (!out) {
        return false;
    }

    out << "# Validation Report\n\n";
    out << "Generated by the deterministic host simulation harness. Hardware commissioning is explicitly out of scope for this report.\n\n";
    out << "## Timing Summary\n\n";
    out << "| Metric | Value |\n";
    out << "| --- | ---: |\n";
    out << "| Cycles | " << metrics.cycles.size() << " |\n";
    out << "| Min cycle (ns) | " << metrics.min_cycle_ns << " |\n";
    out << "| Max cycle (ns) | " << metrics.max_cycle_ns << " |\n";
    out << "| Mean cycle (ns) | " << static_cast<long long>(metrics.mean_cycle_ns) << " |\n";
    out << "| Min jitter (ns) | " << metrics.min_jitter_ns << " |\n";
    out << "| Max jitter (ns) | " << metrics.max_jitter_ns << " |\n";
    out << "| Mean abs jitter (ns) | " << static_cast<long long>(metrics.mean_abs_jitter_ns) << " |\n";
    out << "| Missed cycles | " << metrics.missed_cycles << " |\n";
    out << "| Deadline overruns | " << metrics.deadline_overruns << " |\n\n";

    out << "## Motion Performance\n\n";
    out << "| Metric | Value |\n";
    out << "| --- | ---: |\n";
    out << "| Target position | " << metrics.motion.target_position << " |\n";
    out << "| Final position | " << metrics.motion.final_position << " |\n";
    out << "| Final velocity | " << metrics.motion.final_velocity << " |\n";
    out << "| Overshoot | " << metrics.motion.overshoot << " |\n";
    out << "| Settling time (s) | " << metrics.motion.settling_time_s << " |\n";
    out << "| Steady-state error | " << metrics.motion.steady_state_error << " |\n";
    out << "| Max abs following error | " << metrics.motion.max_abs_following_error << " |\n\n";

    if (metrics.two_axis.available) {
        out << "## Two-Axis Performance\n\n";
        out << "| Metric | Value |\n";
        out << "| --- | ---: |\n";
        out << "| Cycle count | " << metrics.two_axis.cycle_count << " |\n";
        out << "| Acceptance pass | " << (metrics.two_axis.acceptance_pass ? "true" : "false") << " |\n";
        out << "| Target X | " << metrics.two_axis.target_x << " |\n";
        out << "| Target Y | " << metrics.two_axis.target_y << " |\n";
        out << "| Final X | " << metrics.two_axis.final_x << " |\n";
        out << "| Final Y | " << metrics.two_axis.final_y << " |\n";
        out << "| Final position error | " << metrics.two_axis.final_position_error << " |\n";
        out << "| Target tolerance | " << metrics.two_axis.target_tolerance << " |\n";
        out << "| RMS path error | " << metrics.two_axis.rms_path_error << " |\n";
        out << "| Max path error | " << metrics.two_axis.max_path_error << " |\n";
        out << "| Path error limit | " << metrics.two_axis.path_error_limit << " |\n";
        out << "| Settling time (s) | " << metrics.two_axis.settling_time_s << " |\n";
        out << "| Settling time limit (s) | " << metrics.two_axis.settling_time_limit_s << " |\n\n";
    }

    if (metrics.recovery.available) {
        out << "## Fault Recovery Performance\n\n";
        out << "| Metric | Value |\n";
        out << "| --- | ---: |\n";
        out << "| Fault observed | " << (metrics.recovery.fault_observed ? "true" : "false") << " |\n";
        out << "| Operation re-enabled | " << (metrics.recovery.operation_reenabled ? "true" : "false") << " |\n";
        out << "| Post-recovery motion settled | " << (metrics.recovery.post_recovery_motion_settled ? "true" : "false") << " |\n";
        out << "| Detection latency (cycles) | " << metrics.recovery.detection_latency_cycles << " |\n";
        out << "| Recovery to operation enabled (cycles) | " << metrics.recovery.recovery_cycles << " |\n";
        out << "| Baseline settling time (s) | " << metrics.recovery.baseline_settling_time_s << " |\n";
        out << "| Post-recovery settling time (s) | " << metrics.recovery.post_recovery_settling_time_s << " |\n";
        out << "| Settling-time ratio | " << metrics.recovery.settling_time_ratio << " |\n";
        out << "| Post-recovery final error | " << metrics.recovery.post_recovery_final_error << " |\n\n";
    }

    out << "## Bus Diagnostics\n\n";
    out << "| Counter | Value |\n";
    out << "| --- | ---: |\n";
    out << "| TX packets | " << metrics.bus.tx_packets << " |\n";
    out << "| RX packets | " << metrics.bus.rx_packets << " |\n";
    out << "| Packet loss | " << metrics.bus.packet_loss_count << " |\n";
    out << "| Dropout cycles | " << metrics.bus.dropout_count << " |\n";
    out << "| Invalid responses | " << metrics.bus.invalid_response_count << " |\n";
    out << "| Cable disconnect events | " << metrics.bus.cable_disconnect_events << " |\n\n";

    out << "## Scenario Pass/Fail\n\n";
    out << "| Scenario | Result | Evidence | Recovery cycles |\n";
    out << "| --- | --- | --- | ---: |\n";
    for (const auto& scenario : metrics.scenarios) {
        out << "| " << scenario.name
            << " | " << to_string(scenario.result)
            << " | " << scenario.evidence
            << " | " << scenario.recovery_cycles << " |\n";
    }

    out << "\n## Fault Log\n\n";
    if (metrics.fault_log.empty()) {
        out << "No faults recorded.\n";
    } else {
        out << "| Cycle | Fault | Detail |\n";
        out << "| ---: | --- | --- |\n";
        for (const auto& fault : metrics.fault_log) {
            out << "| " << fault.cycle << " | " << to_string(fault.fault)
                << " | " << fault.detail << " |\n";
        }
    }

    return true;
}

}  // namespace emrtc
