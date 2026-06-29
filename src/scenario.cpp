#include "emrtc/scenario.hpp"

#include "emrtc/json.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace emrtc {
namespace {

const JsonValue& required(const JsonValue::Object& object,
                          const char* key,
                          const std::string& context) {
    const auto item = object.find(key);
    if (item == object.end()) {
        throw std::runtime_error(context + " is missing required field '" + std::string(key) + "'");
    }
    return item->second;
}

const JsonValue* optional(const JsonValue::Object& object, const std::string& key) {
    const auto item = object.find(key);
    return item == object.end() ? nullptr : &item->second;
}

void reject_unknown(const JsonValue::Object& object,
                    const std::set<std::string>& allowed,
                    const std::string& context) {
    for (const auto& item : object) {
        if (allowed.count(item.first) == 0U) {
            throw std::runtime_error(context + " contains unknown field '" + item.first + "'");
        }
    }
}

std::uint64_t unsigned_integer(const JsonValue& value,
                               const std::string& context,
                               std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max()) {
    const double number = value.as_number(context);
    if (number < 0.0 || std::floor(number) != number ||
        number > static_cast<double>(maximum)) {
        throw std::runtime_error(context + " must be an unsigned integer in range");
    }
    return static_cast<std::uint64_t>(number);
}

double finite_number(const JsonValue& value, const std::string& context) {
    const double number = value.as_number(context);
    if (!std::isfinite(number)) {
        throw std::runtime_error(context + " must be finite");
    }
    return number;
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

void parse_controller_config(const JsonValue::Object& object,
                             const std::string& context,
                             ControllerConfig& config) {
    if (const auto* value = optional(object, "cycle_time_ns")) {
        config.cycle_time_ns = static_cast<Nanoseconds>(
            unsigned_integer(*value, context + ".cycle_time_ns",
                             static_cast<std::uint64_t>(std::numeric_limits<Nanoseconds>::max())));
    }
    if (const auto* value = optional(object, "deadline_tolerance_ns")) {
        config.deadline_tolerance_ns = static_cast<Nanoseconds>(
            unsigned_integer(*value, context + ".deadline_tolerance_ns",
                             static_cast<std::uint64_t>(std::numeric_limits<Nanoseconds>::max())));
    }
    if (const auto* value = optional(object, "loop_watchdog_limit_cycles")) {
        config.loop_watchdog_limit_cycles = static_cast<std::uint32_t>(
            unsigned_integer(*value, context + ".loop_watchdog_limit_cycles", 0xFFFFFFFFU));
    }
    if (const auto* value = optional(object, "heartbeat_timeout_cycles")) {
        config.heartbeat_timeout_cycles = static_cast<std::uint32_t>(
            unsigned_integer(*value, context + ".heartbeat_timeout_cycles", 0xFFFFFFFFU));
    }
    if (const auto* value = optional(object, "following_error_limit")) {
        config.following_error_limit = finite_number(*value, context + ".following_error_limit");
    }
    if (const auto* value = optional(object, "encoder_loss_timeout_s")) {
        config.encoder_loss_timeout_s = finite_number(*value, context + ".encoder_loss_timeout_s");
    }
    if (const auto* value = optional(object, "max_velocity_units_per_s")) {
        config.max_velocity_units_per_s = finite_number(*value, context + ".max_velocity_units_per_s");
    }
    if (const auto* value = optional(object, "max_accel_units_per_s2")) {
        config.max_accel_units_per_s2 = finite_number(*value, context + ".max_accel_units_per_s2");
    }

    if (config.cycle_time_ns <= 0 || config.deadline_tolerance_ns < 0 ||
        config.loop_watchdog_limit_cycles == 0U || config.heartbeat_timeout_cycles == 0U ||
        config.following_error_limit <= 0.0 || config.encoder_loss_timeout_s <= 0.0 ||
        config.max_velocity_units_per_s <= 0.0 || config.max_accel_units_per_s2 <= 0.0) {
        throw std::runtime_error(context + " contains non-positive controller limits");
    }
}

ScenarioSummary evaluate_expectations(const ScenarioDefinition& scenario,
                                      const RuntimeMetrics& metrics) {
    bool pass = true;
    std::ostringstream evidence;
    pass = pass && metrics.missed_cycles >= scenario.expected.missed_cycles_min;
    pass = pass && metrics.deadline_overruns >= scenario.expected.deadline_overruns_min;
    pass = pass && metrics.bus.packet_loss_count >= scenario.expected.packet_loss_count_min;
    pass = pass &&
           metrics.bus.heartbeat_timeout_count >= scenario.expected.heartbeat_timeout_count_min;
    pass = pass &&
           metrics.bus.cable_disconnect_events >= scenario.expected.cable_disconnect_events_min;
    if (scenario.expected.final_fault != FaultCode::None) {
        const auto final_fault = metrics.trace.empty() ? FaultCode::None : metrics.trace.back().fault;
        pass = pass && final_fault == scenario.expected.final_fault;
    }

    evidence << "missed=" << metrics.missed_cycles
             << " overruns=" << metrics.deadline_overruns
             << " packet_loss=" << metrics.bus.packet_loss_count
             << " heartbeat_timeouts=" << metrics.bus.heartbeat_timeout_count
             << " cable_disconnects=" << metrics.bus.cable_disconnect_events;
    return {scenario.name, pass ? ScenarioResult::Pass : ScenarioResult::Fail,
            evidence.str(), 0};
}

}  // namespace

ScenarioDefinition load_scenario_file(const std::string& path) {
    const auto root = load_json_file(path);
    const auto& object = root.as_object(path);
    reject_unknown(object,
                   {"name", "cycle_time_ns", "deadline_tolerance_ns",
                    "loop_watchdog_limit_cycles", "heartbeat_timeout_cycles",
                    "following_error_limit", "encoder_loss_timeout_s",
                    "max_velocity_units_per_s", "max_accel_units_per_s2",
                    "axis_count", "run_cycles", "command_events",
                    "fault_injections", "expected_evidence"},
                   path);

    ScenarioDefinition scenario;
    if (const auto* name = optional(object, "name")) {
        scenario.name = name->as_string(path + ".name");
    } else {
        scenario.name = std::filesystem::path(path).stem().string();
    }
    if (scenario.name.empty()) throw std::runtime_error(path + ".name must not be empty");

    parse_controller_config(object, path, scenario.config);
    scenario.run_cycles = unsigned_integer(
        required(object, "run_cycles", path), path + ".run_cycles");
    if (scenario.run_cycles == 0U) throw std::runtime_error(path + ".run_cycles must be positive");
    if (const auto* axis_count = optional(object, "axis_count")) {
        if (unsigned_integer(*axis_count, path + ".axis_count") != 1U) {
            throw std::runtime_error(path + ".axis_count must be 1 for this scenario runner");
        }
    }

    const std::string commands_context = path + ".command_events";
    const JsonValue& commands_value = required(object, "command_events", path);
    const auto& commands = commands_value.as_array(commands_context);
    std::set<CycleIndex> command_cycles;
    for (std::size_t index = 0; index < commands.size(); ++index) {
        const std::string context = path + ".command_events[" + std::to_string(index) + ']';
        const auto& command = commands[index].as_object(context);
        reject_unknown(command, {"cycle", "type", "value"}, context);
        ScenarioCommandEvent event;
        event.cycle = unsigned_integer(required(command, "cycle", context), context + ".cycle");
        event.command.type = parse_command_type(
            required(command, "type", context).as_string(context + ".type"));
        if (const auto* value = optional(command, "value")) {
            event.command.value = finite_number(*value, context + ".value");
        }
        if (event.cycle >= scenario.run_cycles) {
            throw std::runtime_error(context + ".cycle must be less than run_cycles");
        }
        if (!command_cycles.insert(event.cycle).second) {
            throw std::runtime_error(context + ".cycle duplicates another command event");
        }
        scenario.commands.push_back(event);
    }

    const std::string faults_context = path + ".fault_injections";
    const JsonValue& faults_value = required(object, "fault_injections", path);
    const auto& faults = faults_value.as_array(faults_context);
    for (std::size_t index = 0; index < faults.size(); ++index) {
        const std::string context = path + ".fault_injections[" + std::to_string(index) + ']';
        const auto& fault = faults[index].as_object(context);
        reject_unknown(fault, {"cycle", "kind", "magnitude", "duration_cycles"}, context);
        FaultInjectionEvent event;
        event.cycle = unsigned_integer(required(fault, "cycle", context), context + ".cycle");
        event.kind = parse_fault_kind(
            required(fault, "kind", context).as_string(context + ".kind"));
        if (const auto* magnitude = optional(fault, "magnitude")) {
            event.magnitude = finite_number(*magnitude, context + ".magnitude");
        }
        if (const auto* duration = optional(fault, "duration_cycles")) {
            event.duration_cycles = static_cast<std::uint32_t>(
                unsigned_integer(*duration, context + ".duration_cycles", 0xFFFFFFFFU));
        }
        if (event.cycle >= scenario.run_cycles || event.duration_cycles == 0U) {
            throw std::runtime_error(context + " must have an in-range cycle and positive duration");
        }
        scenario.faults.push_back(event);
    }

    const std::string expected_context = path + ".expected_evidence";
    const JsonValue& expected_value = required(object, "expected_evidence", path);
    const auto& expected = expected_value.as_object(expected_context);
    reject_unknown(expected,
                   {"missed_cycles_min", "deadline_overruns_min", "packet_loss_count_min",
                    "heartbeat_timeout_count_min", "cable_disconnect_events_min",
                    "final_fault", "hardware_required"},
                   path + ".expected_evidence");
    if (const auto* value = optional(expected, "missed_cycles_min"))
        scenario.expected.missed_cycles_min = unsigned_integer(*value, path + ".expected_evidence.missed_cycles_min");
    if (const auto* value = optional(expected, "deadline_overruns_min"))
        scenario.expected.deadline_overruns_min = unsigned_integer(*value, path + ".expected_evidence.deadline_overruns_min");
    if (const auto* value = optional(expected, "packet_loss_count_min"))
        scenario.expected.packet_loss_count_min = unsigned_integer(*value, path + ".expected_evidence.packet_loss_count_min");
    if (const auto* value = optional(expected, "heartbeat_timeout_count_min"))
        scenario.expected.heartbeat_timeout_count_min = unsigned_integer(*value, path + ".expected_evidence.heartbeat_timeout_count_min");
    if (const auto* value = optional(expected, "cable_disconnect_events_min"))
        scenario.expected.cable_disconnect_events_min = unsigned_integer(*value, path + ".expected_evidence.cable_disconnect_events_min");
    if (const auto* value = optional(expected, "final_fault"))
        scenario.expected.final_fault = parse_fault_code(value->as_string(path + ".expected_evidence.final_fault"));
    if (const auto* value = optional(expected, "hardware_required")) {
        if (value->as_bool(path + ".expected_evidence.hardware_required")) {
            throw std::runtime_error(path + " is a host scenario and cannot require hardware");
        }
    }
    return scenario;
}

RuntimeMetrics run_scenario_definition(const ScenarioDefinition& scenario) {
    RealTimeController controller(scenario.config);
    for (const auto& fault : scenario.faults) controller.fault_injector().schedule(fault);

    auto commands = scenario.commands;
    std::sort(commands.begin(), commands.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.cycle < rhs.cycle;
    });
    std::size_t command_index = 0;
    for (CycleIndex cycle = 0; cycle < scenario.run_cycles; ++cycle) {
        if (command_index < commands.size() && commands[command_index].cycle == cycle) {
            controller.command(commands[command_index].command);
            ++command_index;
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
