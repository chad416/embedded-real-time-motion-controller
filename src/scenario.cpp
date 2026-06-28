#include "emrtc/scenario.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace emrtc {
namespace {

std::string read_all(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open scenario file: " + path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string string_value(const std::string& text, const std::string& key, const std::string& fallback = "") {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    return std::regex_search(text, match, pattern) ? match[1].str() : fallback;
}

std::uint64_t uint_value(const std::string& text, const std::string& key, std::uint64_t fallback = 0) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*([0-9]+)");
    std::smatch match;
    return std::regex_search(text, match, pattern) ? static_cast<std::uint64_t>(std::stoull(match[1].str())) : fallback;
}

double double_value(const std::string& text, const std::string& key, double fallback = 0.0) {
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(-?[0-9]+(?:\\.[0-9]+)?)");
    std::smatch match;
    return std::regex_search(text, match, pattern) ? std::stod(match[1].str()) : fallback;
}

std::string array_block(const std::string& text, const std::string& key) {
    const auto key_pos = text.find("\"" + key + "\"");
    if (key_pos == std::string::npos) {
        return "";
    }
    const auto start = text.find('[', key_pos);
    if (start == std::string::npos) {
        return "";
    }

    int depth = 0;
    for (std::size_t i = start; i < text.size(); ++i) {
        if (text[i] == '[') {
            depth++;
        } else if (text[i] == ']') {
            depth--;
            if (depth == 0) {
                return text.substr(start + 1, i - start - 1);
            }
        }
    }
    return "";
}

std::vector<std::string> object_blocks(const std::string& array_text) {
    std::vector<std::string> result;
    int depth = 0;
    std::size_t start = std::string::npos;
    for (std::size_t i = 0; i < array_text.size(); ++i) {
        if (array_text[i] == '{') {
            if (depth == 0) {
                start = i;
            }
            depth++;
        } else if (array_text[i] == '}') {
            depth--;
            if (depth == 0 && start != std::string::npos) {
                result.push_back(array_text.substr(start, i - start + 1));
                start = std::string::npos;
            }
        }
    }
    return result;
}

FaultKind parse_fault_kind(const std::string& value) {
    if (value == "missed_cycle") return FaultKind::MissedCycle;
    if (value == "cpu_load_spike") return FaultKind::CpuLoadSpike;
    if (value == "heartbeat_timeout") return FaultKind::HeartbeatTimeout;
    if (value == "bus_dropout") return FaultKind::BusDropout;
    if (value == "packet_loss") return FaultKind::PacketLoss;
    if (value == "invalid_drive_transition") return FaultKind::InvalidDriveTransition;
    if (value == "encoder_feedback_loss") return FaultKind::EncoderFeedbackLoss;
    if (value == "sensor_noise") return FaultKind::SensorNoise;
    if (value == "following_error") return FaultKind::FollowingError;
    if (value == "drive_fault") return FaultKind::DriveFault;
    if (value == "watchdog_timeout") return FaultKind::WatchdogTimeout;
    if (value == "command_abuse") return FaultKind::CommandAbuse;
    throw std::runtime_error("unknown fault kind: " + value);
}

CommandType parse_command_type(const std::string& value) {
    if (value == "enable") return CommandType::Enable;
    if (value == "disable") return CommandType::Disable;
    if (value == "fault_reset") return CommandType::FaultReset;
    if (value == "quick_stop") return CommandType::QuickStop;
    if (value == "move_absolute") return CommandType::MoveAbsolute;
    if (value == "move_relative") return CommandType::MoveRelative;
    if (value == "velocity") return CommandType::Velocity;
    if (value == "stop") return CommandType::Stop;
    if (value == "safe_stop") return CommandType::SafeStop;
    throw std::runtime_error("unknown command type: " + value);
}

FaultCode parse_fault_code(const std::string& value) {
    if (value.empty() || value == "none") return FaultCode::None;
    if (value == "heartbeat_timeout") return FaultCode::HeartbeatTimeout;
    if (value == "bus_dropout") return FaultCode::BusDropout;
    if (value == "following_error") return FaultCode::FollowingError;
    if (value == "encoder_feedback_loss") return FaultCode::EncoderFeedbackLoss;
    if (value == "drive_fault") return FaultCode::DriveFault;
    if (value == "watchdog_timeout") return FaultCode::WatchdogTimeout;
    if (value == "command_rejected") return FaultCode::CommandRejected;
    if (value == "emergency_stop") return FaultCode::EmergencyStop;
    if (value == "invalid_drive_transition") return FaultCode::InvalidDriveTransition;
    throw std::runtime_error("unknown expected final fault: " + value);
}

ScenarioSummary evaluate_expectations(const ScenarioDefinition& scenario, const RuntimeMetrics& metrics) {
    bool pass = true;
    std::ostringstream evidence;

    if (metrics.missed_cycles < scenario.expected.missed_cycles_min) {
        pass = false;
    }
    if (metrics.deadline_overruns < scenario.expected.deadline_overruns_min) {
        pass = false;
    }
    if (metrics.bus.packet_loss_count < scenario.expected.packet_loss_count_min) {
        pass = false;
    }
    if (metrics.bus.heartbeat_timeout_count < scenario.expected.heartbeat_timeout_count_min) {
        pass = false;
    }
    if (metrics.bus.cable_disconnect_events < scenario.expected.cable_disconnect_events_min) {
        pass = false;
    }
    if (scenario.expected.final_fault != FaultCode::None) {
        const auto final_fault = metrics.trace.empty() ? FaultCode::None : metrics.trace.back().fault;
        if (final_fault != scenario.expected.final_fault) {
            pass = false;
        }
    }

    evidence << "missed=" << metrics.missed_cycles
             << " overruns=" << metrics.deadline_overruns
             << " packet_loss=" << metrics.bus.packet_loss_count
             << " heartbeat_timeouts=" << metrics.bus.heartbeat_timeout_count
             << " cable_disconnects=" << metrics.bus.cable_disconnect_events;

    return {scenario.name, pass ? ScenarioResult::Pass : ScenarioResult::Fail, evidence.str(), 0};
}

}  // namespace

ScenarioDefinition load_scenario_file(const std::string& path) {
    const auto text = read_all(path);
    ScenarioDefinition scenario;
    scenario.name = string_value(text, "name", std::filesystem::path(path).stem().string());
    scenario.config.cycle_time_ns = static_cast<Nanoseconds>(uint_value(text, "cycle_time_ns", scenario.config.cycle_time_ns));
    scenario.run_cycles = uint_value(text, "run_cycles", scenario.run_cycles);

    for (const auto& object : object_blocks(array_block(text, "fault_injections"))) {
        FaultInjectionEvent event;
        event.cycle = uint_value(object, "cycle", 0);
        event.kind = parse_fault_kind(string_value(object, "kind"));
        event.magnitude = double_value(object, "magnitude", 1.0);
        event.duration_cycles = static_cast<std::uint32_t>(uint_value(object, "duration_cycles", 1));
        scenario.faults.push_back(event);
    }

    for (const auto& object : object_blocks(array_block(text, "command_events"))) {
        ScenarioCommandEvent event;
        event.cycle = uint_value(object, "cycle", 0);
        event.command.type = parse_command_type(string_value(object, "type"));
        event.command.value = double_value(object, "value", 0.0);
        scenario.commands.push_back(event);
    }

    scenario.expected.missed_cycles_min = uint_value(text, "missed_cycles_min", 0);
    scenario.expected.deadline_overruns_min = uint_value(text, "deadline_overruns_min", 0);
    scenario.expected.packet_loss_count_min = uint_value(text, "packet_loss_count_min", 0);
    scenario.expected.heartbeat_timeout_count_min = uint_value(text, "heartbeat_timeout_count_min", 0);
    scenario.expected.cable_disconnect_events_min = uint_value(text, "cable_disconnect_events_min", 0);
    scenario.expected.final_fault = parse_fault_code(string_value(text, "final_fault", "none"));
    return scenario;
}

RuntimeMetrics run_scenario_definition(const ScenarioDefinition& scenario) {
    RealTimeController controller(scenario.config);
    for (const auto& fault : scenario.faults) {
        controller.fault_injector().schedule(fault);
    }

    auto commands = scenario.commands;
    std::sort(commands.begin(), commands.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.cycle < rhs.cycle;
    });

    std::size_t command_index = 0;
    for (CycleIndex cycle = 0; cycle < scenario.run_cycles; ++cycle) {
        while (command_index < commands.size() && commands[command_index].cycle == cycle) {
            controller.command(commands[command_index].command);
            command_index++;
        }
        controller.run_cycles(1);
    }

    auto metrics = controller.metrics();
    metrics.scenarios.push_back(evaluate_expectations(scenario, metrics));
    return metrics;
}

RuntimeMetrics run_scenario_file(const std::string& path) {
    return run_scenario_definition(load_scenario_file(path));
}

std::vector<std::string> list_scenario_files(const std::string& directory) {
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path().string());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

}  // namespace emrtc
