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

    out << "cycle,timestamp_ns,event,state,mode,fault,position,velocity,following_error\n";
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
