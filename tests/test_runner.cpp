#include "emrtc/adapter_contract.hpp"
#include "emrtc/adapter_config.hpp"
#include "emrtc/controller.hpp"
#include "emrtc/hardware_adapters.hpp"
#include "emrtc/json.hpp"
#include "emrtc/multi_axis.hpp"
#include "emrtc/process_image_mapping.hpp"
#include "emrtc/replay_adapter.hpp"
#include "emrtc/realtime.hpp"
#include "emrtc/scenario.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct TestResult {
    std::string name;
    bool pass{false};
    std::string detail;
};

class ExternalLoopbackBackend final : public emrtc::IBusBackend {
public:
    emrtc::BusExchangeResult exchange(
        emrtc::CycleIndex cycle,
        std::vector<emrtc::ProcessImage>& images) override {
        diagnostics_.tx_packets++;
        diagnostics_.rx_packets++;
        diagnostics_.last_heartbeat_cycle = cycle;
        diagnostics_.online = true;
        diagnostics_.heartbeat_ok = true;

        if (images.size() != 1U) {
            diagnostics_.invalid_response_count++;
            return {false, true, true};
        }

        observed_controlwords.push_back(images.front().controlword);
        observed_target_positions.push_back(images.front().target_position);
        if (images.front().controlword == 0x0006U) {
            statusword_ = 0x0021U;
        } else if (images.front().controlword == 0x0007U) {
            statusword_ = 0x0023U;
        } else if (images.front().controlword == 0x000FU) {
            statusword_ = 0x0027U;
        } else if (images.front().controlword == 0x0080U) {
            statusword_ = 0x0040U;
        }

        images.front().statusword = statusword_;
        images.front().actual_position = actual_position;
        images.front().actual_velocity = actual_velocity;
        images.front().drive_fault = emrtc::FaultCode::None;
        return {};
    }

    const emrtc::BusDiagnostics& diagnostics() const override {
        return diagnostics_;
    }

    double actual_position{1.25};
    double actual_velocity{0.5};
    std::vector<std::uint16_t> observed_controlwords;
    std::vector<double> observed_target_positions;

private:
    emrtc::BusDiagnostics diagnostics_;
    std::uint16_t statusword_{0x0040U};
};

class FakeCanTransport final : public emrtc::ICanTransport {
public:
    bool open(const std::string& interface_name, std::string& detail) override {
        opened_interface = interface_name;
        open_ = allow_open;
        detail = open_ ? "fake open" : "fake open failure";
        return open_;
    }

    void close() override {
        open_ = false;
    }

    bool send(const emrtc::CanFrame& frame, std::string& detail) override {
        if (!open_ || fail_send) {
            detail = "fake send failure";
            return false;
        }
        sent.push_back(frame);
        detail = "ok";
        return true;
    }

    emrtc::CanReceiveStatus receive(emrtc::CanFrame& frame,
                                    std::uint32_t timeout_us,
                                    std::string& detail) override {
        observed_timeouts.push_back(timeout_us);
        if (!open_ || fail_receive) {
            detail = "fake receive failure";
            return emrtc::CanReceiveStatus::Error;
        }
        if (incoming.empty()) {
            detail = "timeout";
            return emrtc::CanReceiveStatus::Timeout;
        }
        frame = incoming.front();
        incoming.pop_front();
        detail = "ok";
        return emrtc::CanReceiveStatus::Frame;
    }

    bool is_open() const override {
        return open_;
    }

    bool allow_open{true};
    bool fail_send{false};
    bool fail_receive{false};
    std::string opened_interface;
    std::vector<emrtc::CanFrame> sent;
    std::deque<emrtc::CanFrame> incoming;
    std::vector<std::uint32_t> observed_timeouts;

private:
    bool open_{false};
};

void write_test_u16(emrtc::CanFrame& frame, std::size_t offset, std::uint16_t value) {
    frame.data[offset] = static_cast<std::uint8_t>(value & 0xFFU);
    frame.data[offset + 1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

void write_test_i32(emrtc::CanFrame& frame, std::size_t offset, std::int32_t value) {
    const auto encoded = static_cast<std::uint32_t>(value);
    for (std::size_t index = 0; index < 4U; ++index) {
        frame.data[offset + index] =
            static_cast<std::uint8_t>((encoded >> (index * 8U)) & 0xFFU);
    }
}

emrtc::CanFrame make_tpdo1(const emrtc::SocketCanCanOpenConfig& config,
                           std::uint16_t statusword,
                           std::int32_t position_counts) {
    emrtc::CanFrame frame;
    frame.id = config.tpdo1_cob_id;
    frame.dlc = 6U;
    write_test_u16(frame, 0U, statusword);
    write_test_i32(frame, 2U, position_counts);
    return frame;
}

emrtc::CanFrame make_tpdo2(const emrtc::SocketCanCanOpenConfig& config,
                           std::int32_t velocity_counts) {
    emrtc::CanFrame frame;
    frame.id = config.tpdo2_cob_id;
    frame.dlc = 4U;
    write_test_i32(frame, 0U, velocity_counts);
    return frame;
}

emrtc::CanFrame make_heartbeat(const emrtc::SocketCanCanOpenConfig& config,
                               std::uint8_t state = 0x05U) {
    emrtc::CanFrame frame;
    frame.id = config.heartbeat_cob_id;
    frame.dlc = 1U;
    frame.data[0] = state;
    return frame;
}

bool near(double value, double target, double tolerance) {
    return std::abs(value - target) <= tolerance;
}

void enable_axis(emrtc::RealTimeController& controller) {
    controller.command({emrtc::CommandType::Enable, 0.0});
    controller.run_cycles(5);
}

TestResult test_normal_startup() {
    emrtc::RealTimeController c;
    c.run_cycles(1);
    return {"normal startup", c.axis().state() == emrtc::DriveState::SwitchOnDisabled,
            "boot reaches Switch On Disabled"};
}

TestResult test_drive_enable_sequence() {
    emrtc::RealTimeController c;
    enable_axis(c);
    return {"drive enable sequence", c.axis().state() == emrtc::DriveState::OperationEnabled,
            "CiA 402 enable sequence reaches Operation Enabled"};
}

TestResult test_absolute_move() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.command({emrtc::CommandType::MoveAbsolute, 1.0});
    c.run_cycles(900);
    return {"absolute move", near(c.axis().feedback().position, 1.0, 0.01),
            "axis settles at absolute target"};
}

TestResult test_relative_move() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.command({emrtc::CommandType::MoveRelative, 0.5});
    c.run_cycles(600);
    return {"relative move", near(c.axis().feedback().position, 0.5, 0.01),
            "axis settles at relative target"};
}

TestResult test_velocity_move() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.command({emrtc::CommandType::Velocity, 0.75});
    c.run_cycles(100);
    return {"velocity move", c.axis().feedback().velocity > 0.5,
            "velocity ramps toward command"};
}

TestResult test_quick_stop() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.command({emrtc::CommandType::Velocity, 1.0});
    c.run_cycles(50);
    c.command({emrtc::CommandType::QuickStop, 0.0});
    c.run_cycles(100);
    return {"quick stop during motion",
            c.axis().state() == emrtc::DriveState::QuickStopActive &&
                std::abs(c.axis().feedback().velocity) < 0.01,
            "quick stop arrests motion"};
}

TestResult test_estop() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.command({emrtc::CommandType::Velocity, 1.0});
    c.run_cycles(30);
    c.request_estop();
    c.run_cycles(100);
    return {"E-stop safe stop during motion",
            c.axis().fault_code() == emrtc::FaultCode::EmergencyStop,
            "safe stop records EmergencyStop fault code"};
}

TestResult test_fault_reset_recovery() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.fault_injector().schedule({8, emrtc::FaultKind::DriveFault, 1.0, 1});
    c.run_cycles(12);
    c.reset_faults();
    c.command({emrtc::CommandType::Enable, 0.0});
    c.run_cycles(5);
    return {"fault reset and recovery", c.axis().state() == emrtc::DriveState::OperationEnabled,
            "fault reset permits re-enable"};
}

TestResult test_invalid_transition() {
    emrtc::RealTimeController c;
    c.fault_injector().schedule({1, emrtc::FaultKind::InvalidDriveTransition, 1.0, 1});
    c.run_cycles(3);
    return {"invalid state transition rejected",
            c.axis().fault_code() == emrtc::FaultCode::InvalidDriveTransition,
            "invalid transition enters fault"};
}

TestResult test_missed_cycle() {
    emrtc::RealTimeController c;
    c.fault_injector().schedule({3, emrtc::FaultKind::MissedCycle, 0.0, 1});
    c.run_cycles(8);
    return {"missed-cycle injection", c.metrics().missed_cycles == 1,
            "missed cycle counter increments"};
}

TestResult test_cpu_load_jitter() {
    emrtc::RealTimeController c;
    c.fault_injector().schedule({3, emrtc::FaultKind::CpuLoadSpike, 1.5, 1});
    c.run_cycles(8);
    return {"CPU load jitter stress", c.metrics().deadline_overruns >= 1,
            "deadline overrun captured"};
}

TestResult test_heartbeat_timeout() {
    emrtc::RealTimeController c;
    c.fault_injector().schedule({2, emrtc::FaultKind::HeartbeatTimeout, 1.0, 8});
    c.run_cycles(12);
    return {"heartbeat timeout", c.axis().fault_code() == emrtc::FaultCode::HeartbeatTimeout,
            "heartbeat watchdog trips"};
}

TestResult test_bus_disconnect() {
    emrtc::RealTimeController c;
    c.fault_injector().schedule({2, emrtc::FaultKind::BusDropout, 1.0, 4});
    c.run_cycles(8);
    return {"bus cable disconnect simulation",
            c.metrics().bus.cable_disconnect_events == 1 &&
                c.axis().fault_code() == emrtc::FaultCode::BusDropout,
            "bus dropout records disconnect and drive fault"};
}

TestResult test_packet_loss() {
    emrtc::RealTimeController c;
    c.fault_injector().schedule({2, emrtc::FaultKind::PacketLoss, 2.0, 6});
    c.run_cycles(20);
    return {"packet loss simulation", c.metrics().bus.packet_loss_count >= 1,
            "packet loss counter increments"};
}

TestResult test_encoder_loss() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.fault_injector().schedule({10, emrtc::FaultKind::EncoderFeedbackLoss, 1.0, 10});
    c.run_cycles(20);
    return {"encoder feedback loss",
            c.axis().fault_code() == emrtc::FaultCode::EncoderFeedbackLoss,
            "encoder watchdog trips"};
}

TestResult test_following_error() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.fault_injector().schedule({10, emrtc::FaultKind::FollowingError, 0.2, 1});
    c.run_cycles(12);
    return {"following error", c.axis().fault_code() == emrtc::FaultCode::FollowingError,
            "following-error watchdog trips"};
}

TestResult test_command_abuse() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.command({emrtc::CommandType::Velocity, 0.5});
    c.run_cycles(20);
    c.fault_injector().schedule({25, emrtc::FaultKind::CommandAbuse, 1.0, 1});
    c.run_cycles(10);
    return {"command abuse during motion",
            c.axis().fault_code() == emrtc::FaultCode::CommandRejected,
            "invalid command during motion is rejected"};
}

TestResult test_sensor_noise() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.fault_injector().schedule({10, emrtc::FaultKind::SensorNoise, 0.005, 1});
    c.run_cycles(12);
    return {"sensor noise injection",
            c.axis().fault_code() == emrtc::FaultCode::None &&
                c.axis().feedback().position > 0.0,
            "bounded noise is visible and below trip threshold"};
}

TestResult test_warm_restart() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.command({emrtc::CommandType::MoveAbsolute, 0.25});
    c.run_cycles(200);
    c.warm_restart();
    c.command({emrtc::CommandType::Enable, 0.0});
    c.run_cycles(5);
    return {"warm restart", c.axis().state() == emrtc::DriveState::OperationEnabled,
            "warm restart can re-enable"};
}

TestResult test_cold_restart() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.command({emrtc::CommandType::MoveAbsolute, 0.25});
    c.run_cycles(200);
    c.cold_restart();
    c.command({emrtc::CommandType::Enable, 0.0});
    c.run_cycles(5);
    return {"cold restart",
            c.axis().state() == emrtc::DriveState::OperationEnabled &&
                near(c.axis().feedback().position, 0.0, 0.001),
            "cold restart clears simulated position"};
}

TestResult test_fault_log_replay() {
    emrtc::RealTimeController c;
    enable_axis(c);
    c.fault_injector().schedule({10, emrtc::FaultKind::DriveFault, 1.0, 1});
    c.run_cycles(20);
    const auto metrics = c.metrics();
    return {"persistent fault log replay",
            !metrics.fault_log.empty() &&
                metrics.fault_log.back().fault == emrtc::FaultCode::DriveFault,
            "fault log contains replayable event"};
}

TestResult test_external_cia402_feedback_and_controlword_sequence() {
    emrtc::Axis axis("external_axis");
    axis.apply_external_feedback(0x0040U, 0.0, 0.0, emrtc::FaultCode::None, 0);
    const bool accepted = axis.apply_command({emrtc::CommandType::Enable, 0.0}, 0);
    const bool shutdown = axis.feedback().controlword == 0x0006U;

    axis.apply_external_feedback(0x0021U, 0.25, 0.10, emrtc::FaultCode::None, 1);
    const bool switch_on = axis.feedback().controlword == 0x0007U;
    axis.apply_external_feedback(0x0023U, 0.50, 0.20, emrtc::FaultCode::None, 2);
    const bool enable_operation = axis.feedback().controlword == 0x000FU;
    axis.apply_external_feedback(0x0427U, 0.75, 0.30, emrtc::FaultCode::None, 3);
    const auto operational = axis.feedback();

    axis.apply_external_feedback(0x0008U, 0.75, 0.0, emrtc::FaultCode::DriveFault, 4);
    const bool reset_accepted =
        axis.apply_command({emrtc::CommandType::FaultReset, 0.0}, 4);
    const bool reset_pulse = axis.feedback().controlword == 0x0080U;
    axis.apply_external_feedback(0x0008U, 0.75, 0.0, emrtc::FaultCode::DriveFault, 4);
    axis.update(0.001, 4);
    const bool one_cycle_pulse = axis.feedback().controlword == 0x0000U;

    axis.apply_external_feedback(0x0027U, 0.75, 0.0, emrtc::FaultCode::None, 5);
    axis.force_fault(emrtc::FaultCode::FollowingError, 5, "local watchdog fault");
    axis.apply_external_feedback(0x0027U, 0.75, 0.0, emrtc::FaultCode::None, 5);
    axis.update(0.001, 5);
    const bool local_fault_latched =
        axis.state() == emrtc::DriveState::Fault &&
        axis.fault_code() == emrtc::FaultCode::FollowingError;
    const bool local_reset_accepted =
        axis.apply_command({emrtc::CommandType::FaultReset, 0.0}, 6);
    const bool local_reset_pulse = axis.feedback().controlword == 0x0080U;
    axis.apply_external_feedback(0x0040U, 0.75, 0.0, emrtc::FaultCode::None, 6);
    axis.update(0.001, 6);
    const bool local_fault_cleared =
        axis.state() == emrtc::DriveState::SwitchOnDisabled &&
        axis.fault_code() == emrtc::FaultCode::None;

    return {"external CiA 402 feedback and controlword sequence",
            accepted &&
                shutdown &&
                switch_on &&
                enable_operation &&
                operational.state == emrtc::DriveState::OperationEnabled &&
                operational.target_reached &&
                near(operational.position, 0.75, 0.0001) &&
                near(operational.velocity, 0.30, 0.0001) &&
                reset_accepted &&
                reset_pulse &&
                one_cycle_pulse &&
                local_fault_latched &&
                local_reset_accepted &&
                local_reset_pulse &&
                local_fault_cleared,
            "external feedback drives state while local faults stay latched through explicit reset"};
}

TestResult test_controller_uses_injected_external_backend() {
    ExternalLoopbackBackend backend;
    emrtc::RealTimeController controller(backend);
    controller.command({emrtc::CommandType::Enable, 0.0});
    controller.run_cycles(3);
    controller.command({emrtc::CommandType::MoveAbsolute, 2.0});
    controller.run_cycles(1);
    controller.command({emrtc::CommandType::MoveRelative, 0.25});
    controller.run_cycles(1);
    controller.fault_injector().schedule(
        {5, emrtc::FaultKind::PacketLoss, 2.0, 3});
    controller.run_cycles(1);

    bool simulation_api_rejected = false;
    try {
        static_cast<void>(controller.bus());
    } catch (const std::logic_error&) {
        simulation_api_rejected = true;
    }

    const auto feedback = controller.axis().feedback();
    const auto metrics = controller.metrics();
    const bool unavailable_injection_logged =
        std::any_of(metrics.trace.begin(), metrics.trace.end(), [](const auto& event) {
            return event.event ==
                   "simulation-only packet loss injection unavailable with external backend";
        });
    const bool target_forwarded =
        std::any_of(backend.observed_target_positions.begin(),
                    backend.observed_target_positions.end(),
                    [](double value) { return near(value, 2.0, 0.0001); });
    const bool relative_target_uses_measured_position =
        std::any_of(backend.observed_target_positions.begin(),
                    backend.observed_target_positions.end(),
                    [](double value) { return near(value, 1.5, 0.0001); });

    return {"controller injected external backend",
            !controller.using_simulated_backend() &&
                &controller.backend() == &backend &&
                simulation_api_rejected &&
                controller.axis().state() == emrtc::DriveState::OperationEnabled &&
                near(feedback.position, backend.actual_position, 0.0001) &&
                near(feedback.velocity, backend.actual_velocity, 0.0001) &&
                backend.observed_controlwords.size() == 6U &&
                backend.observed_controlwords[0] == 0x0006U &&
                backend.observed_controlwords[1] == 0x0007U &&
                backend.observed_controlwords[2] == 0x000FU &&
                target_forwarded &&
                relative_target_uses_measured_position &&
                metrics.bus.tx_packets == 6U &&
                metrics.bus.rx_packets == 6U &&
                metrics.bus.packet_loss_count == 0U &&
                unavailable_injection_logged,
            "controller exchanges real process images without exposing simulated-bus controls"};
}

TestResult test_json_scenario_loader() {
    const std::string path = "examples/scenarios/fault_matrix.json";
    if (!std::filesystem::exists(path)) {
        return {"JSON scenario loader", false, "scenario file missing"};
    }
    const auto metrics = emrtc::run_scenario_file(path);
    const bool pass = !metrics.scenarios.empty() &&
                      metrics.scenarios.front().result == emrtc::ScenarioResult::Pass &&
                      metrics.bus.packet_loss_count >= 1 &&
                      metrics.deadline_overruns >= 1 &&
                      metrics.missed_cycles >= 1;
    return {"JSON scenario loader", pass, "fault matrix scenario loads and passes expectations"};
}

TestResult test_strict_json_parser() {
    bool duplicate_rejected = false;
    bool trailing_comma_rejected = false;
    try {
        (void)emrtc::parse_json("{\"value\":1,\"value\":2}", "duplicate.json");
    } catch (const std::runtime_error&) {
        duplicate_rejected = true;
    }
    try {
        (void)emrtc::parse_json("[1,2,]", "trailing.json");
    } catch (const std::runtime_error&) {
        trailing_comma_rejected = true;
    }
    const auto valid = emrtc::parse_json(
        "{\"array\":[true,false,null],\"escaped\":\"line\\nvalue\"}",
        "valid.json");
    const auto& object = valid.as_object("valid.json");
    const bool valid_shape =
        object.at("array").as_array("valid.json.array").size() == 3U &&
        object.at("escaped").as_string("valid.json.escaped") == "line\nvalue";
    return {"strict structured JSON parser",
            duplicate_rejected && trailing_comma_rejected && valid_shape,
            "valid JSON is structured while duplicate keys and trailing commas are rejected"};
}

TestResult test_runtime_adapter_config_validation() {
    const auto soem = emrtc::load_runtime_adapter_config(
        "config/adapters/soem_single_axis.example.json");
    const auto canopen = emrtc::load_runtime_adapter_config(
        "config/adapters/socketcan_canopen_single_axis.example.json");
    const auto soem_summary = emrtc::summarize_runtime_adapter_config(soem);
    const auto canopen_summary = emrtc::summarize_runtime_adapter_config(canopen);
    return {"runtime adapter configuration validation",
            soem.backend == emrtc::RuntimeAdapterBackend::SoemEtherCat &&
                soem.soem.observed_mapping.size() == 6U &&
                canopen.backend == emrtc::RuntimeAdapterBackend::SocketCanCanOpen &&
                canopen.socketcan.node_id == 1U &&
                canopen.socketcan.observed_mapping.size() == 6U &&
                soem_summary.find("hardware_access=NONE") != std::string::npos &&
                canopen_summary.find("validation=PASS") != std::string::npos,
            "SOEM and SocketCAN examples validate without opening hardware"};
}

TestResult test_runtime_adapter_config_rejects_unsafe_values() {
    auto config = emrtc::load_runtime_adapter_config(
        "config/adapters/socketcan_canopen_single_axis.example.json");
    config.socketcan.heartbeat_cob_id = config.socketcan.tpdo1_cob_id;
    bool duplicate_id_rejected = false;
    try {
        emrtc::validate_runtime_adapter_config(config);
    } catch (const std::runtime_error&) {
        duplicate_id_rejected = true;
    }
    config = emrtc::load_runtime_adapter_config(
        "config/adapters/soem_single_axis.example.json");
    config.soem.counts_per_unit = std::numeric_limits<double>::quiet_NaN();
    bool nonfinite_scale_rejected = false;
    try {
        emrtc::validate_runtime_adapter_config(config);
    } catch (const std::runtime_error&) {
        nonfinite_scale_rejected = true;
    }
    return {"runtime adapter configuration rejects unsafe values",
            duplicate_id_rejected && nonfinite_scale_rejected,
            "duplicate COB-IDs and non-finite scaling fail before hardware access"};
}

TestResult test_host_realtime_probe_uses_absolute_schedule() {
    emrtc::RealtimeProbeConfig config;
    config.cycles = 20U;
    config.period_ns = 200'000;
    config.deadline_tolerance_ns = 1'000'000;
    std::uint64_t callback_count = 0;
    const auto metrics = emrtc::run_realtime_probe(
        config, [&callback_count](emrtc::CycleIndex) { ++callback_count; });
    bool timestamps_increase = true;
    for (std::size_t index = 1; index < metrics.samples.size(); ++index) {
        timestamps_increase = timestamps_increase &&
                              metrics.samples[index].wake_timestamp_ns >=
                                  metrics.samples[index - 1U].wake_timestamp_ns;
    }
    bool invalid_rejected = false;
    try {
        auto invalid = config;
        invalid.period_ns = 0;
        (void)emrtc::run_realtime_probe(invalid);
    } catch (const std::invalid_argument&) {
        invalid_rejected = true;
    }
    const bool pass = metrics.samples.size() == config.cycles &&
                      callback_count == config.cycles && timestamps_increase &&
                      metrics.max_period_ns > 0 && metrics.mean_period_ns > 0.0 &&
                      metrics.capabilities.monotonic_clock &&
                      metrics.capabilities.absolute_sleep && invalid_rejected;
    return {"host real-time probe absolute scheduling",
            pass,
            "samples=" + std::to_string(metrics.samples.size()) +
                " callbacks=" + std::to_string(callback_count) +
                " min_period_ns=" + std::to_string(metrics.min_period_ns) +
                " timestamps_increase=" + (timestamps_increase ? "true" : "false") +
                " invalid_rejected=" + (invalid_rejected ? "true" : "false")};
}

TestResult test_json_scenario_suite() {
    const auto files = emrtc::list_scenario_files("examples/scenarios");
    if (files.size() < 6) {
        return {"JSON scenario suite", false, "expected at least six scenario files"};
    }

    for (const auto& file : files) {
        const auto metrics = emrtc::run_scenario_file(file);
        if (metrics.scenarios.empty() ||
            metrics.scenarios.front().result != emrtc::ScenarioResult::Pass) {
            return {"JSON scenario suite", false, "scenario failed: " + file};
        }
    }
    return {"JSON scenario suite", true, "all JSON scenarios pass declared expectations"};
}

TestResult test_two_axis_coordinated_move() {
    const auto metrics = emrtc::simulate_two_axis_linear_move();
    return {"two-axis coordinated move",
            metrics.target_reached && metrics.path_within_limit &&
                metrics.acceptance_pass &&
                near(metrics.final_x, 1.0, 0.01) &&
                near(metrics.final_y, 1.0, 0.01) &&
                metrics.final_position_error <= metrics.target_tolerance,
            "two simulated axes settle on a bounded straight-line path within acceptance limits"};
}

TestResult test_hardware_adapter_skeletons_are_not_claiming_commissioning() {
    emrtc::SoemEtherCatBackend soem;
    emrtc::EtherLabEtherCatBackend etherlab;
    emrtc::SocketCanCanOpenBackend socket_can;
    emrtc::CanOpenNodeBackend canopen_node;
    const auto soem_status = soem.status();
    const auto etherlab_status = etherlab.status();
    const auto can_status = socket_can.status();
    const auto canopen_node_status = canopen_node.status();
    const bool uncommissioned = !soem_status.configured && !etherlab_status.configured &&
                                !can_status.configured && !canopen_node_status.configured;
    const bool dependency_flags_are_coherent =
        (!soem_status.dependency_available || soem_status.build_flag_enabled) &&
        (!etherlab_status.dependency_available || etherlab_status.build_flag_enabled) &&
        (!can_status.dependency_available || can_status.build_flag_enabled) &&
        (!canopen_node_status.dependency_available || canopen_node_status.build_flag_enabled);
    return {"hardware adapter skeleton status",
            uncommissioned && dependency_flags_are_coherent &&
                !soem.diagnostics().online &&
                !etherlab.diagnostics().online &&
                !socket_can.diagnostics().online &&
                !canopen_node.diagnostics().online,
            "future hardware adapters report uncommissioned status"};
}

TestResult test_soem_configuration_guards_do_not_touch_hardware() {
    emrtc::SoemEtherCatBackend soem;
    emrtc::SoemAdapterConfig config;
    config.observed_mapping = emrtc::expected_adapter_mapping();

    const auto empty_interface = soem.configure(config);

    config.interface_name = "not-opened-because-mapping-is-invalid";
    config.observed_mapping[0].object_index = 0x0000;
    const auto invalid_mapping = soem.configure(config);

    config.observed_mapping = emrtc::expected_adapter_mapping();
    config.io_map_bytes = 128;
    const auto small_map = soem.configure(config);

    config.io_map_bytes = 4096;
    config.expected_slave_count = 2;
    const auto multiple_slaves = soem.configure(config);

    config.expected_slave_count = 1;
    config.receive_timeout_us = 0;
    const auto zero_timeout = soem.configure(config);

    std::vector<emrtc::ProcessImage> images(1);
    const auto exchange = soem.exchange(0, images);
    return {"SOEM configuration guards do not touch hardware",
            !empty_interface.ok &&
                empty_interface.detail.find("interface") != std::string::npos &&
                !invalid_mapping.ok &&
                invalid_mapping.detail.find("mapping") != std::string::npos &&
                !small_map.ok &&
                small_map.detail.find("4096") != std::string::npos &&
                !multiple_slaves.ok &&
                multiple_slaves.detail.find("expected_slave_count=1") !=
                    std::string::npos &&
                !zero_timeout.ok &&
                zero_timeout.detail.find("receive_timeout_us") !=
                    std::string::npos &&
                !exchange.ok &&
                !soem.status().configured &&
                !soem.diagnostics().online,
            "invalid preconditions fail before NIC discovery or cyclic exchange"};
}

TestResult test_soem_pdo_codec_is_bounded_and_little_endian() {
    emrtc::ProcessImage image;
    image.controlword = 0x1234;
    image.target_position = 1.25;
    image.target_velocity = -0.5;
    std::vector<std::uint8_t> outputs(10, 0);
    const emrtc::SoemPdoLayout layout;
    const auto encoded =
        emrtc::encode_soem_process_outputs(image, 1000.0, layout, outputs.data(), outputs.size());

    std::vector<std::uint8_t> inputs{
        0x2FU, 0x00U,
        0xD2U, 0x04U, 0x00U, 0x00U,
        0x0CU, 0xFEU, 0xFFU, 0xFFU,
    };
    const auto decoded =
        emrtc::decode_soem_process_inputs(inputs.data(),
                                          inputs.size(),
                                          1000.0,
                                          layout,
                                          image);
    const auto bounded =
        emrtc::encode_soem_process_outputs(image, 1000.0, layout, outputs.data(), 4);

    return {"SOEM PDO codec is bounded and little-endian",
            encoded.ok &&
                outputs[0] == 0x34U &&
                outputs[1] == 0x12U &&
                outputs[2] == 0xE2U &&
                outputs[3] == 0x04U &&
                outputs[6] == 0x0CU &&
                outputs[7] == 0xFEU &&
                decoded.ok &&
                image.statusword == 0x002FU &&
                near(image.actual_position, 1.234, 0.001) &&
                near(image.actual_velocity, -0.5, 0.001) &&
                image.drive_fault == emrtc::FaultCode::DriveFault &&
                !bounded.ok,
            "PDO bytes round trip with explicit bounds and statusword fault decoding"};
}

TestResult test_canopen_pdo_codec_is_bounded_and_little_endian() {
    const auto runtime = emrtc::load_runtime_adapter_config(
        "config/adapters/socketcan_canopen_single_axis.example.json");
    const auto& config = runtime.socketcan;
    emrtc::ProcessImage image;
    image.controlword = 0x1234U;
    image.target_position = 1.25;
    image.target_velocity = -0.5;
    const auto rpdo1 = emrtc::encode_canopen_rpdo1(image, config);
    const auto rpdo2 = emrtc::encode_canopen_rpdo2(image, config);

    auto tpdo1 = make_tpdo1(config, 0x0027U, 1500);
    auto tpdo2 = make_tpdo2(config, -250);
    bool received_tpdo1 = false;
    bool received_tpdo2 = false;
    const auto decoded1 = emrtc::decode_canopen_tpdo(
        tpdo1, config, image, received_tpdo1, received_tpdo2);
    const auto decoded2 = emrtc::decode_canopen_tpdo(
        tpdo2, config, image, received_tpdo1, received_tpdo2);
    tpdo1.dlc = 5U;
    const auto short_frame = emrtc::decode_canopen_tpdo(
        tpdo1, config, image, received_tpdo1, received_tpdo2);

    return {"CANopen PDO codec is bounded and little-endian",
            rpdo1.id == config.rpdo1_cob_id && rpdo1.dlc == 6U &&
                rpdo1.data[0] == 0x34U && rpdo1.data[1] == 0x12U &&
                rpdo1.data[2] == 0xE2U && rpdo1.data[3] == 0x04U &&
                rpdo2.id == config.rpdo2_cob_id && rpdo2.dlc == 4U &&
                rpdo2.data[0] == 0x0CU && rpdo2.data[1] == 0xFEU &&
                rpdo2.data[2] == 0xFFU && rpdo2.data[3] == 0xFFU &&
                decoded1.ok && decoded2.ok && received_tpdo1 && received_tpdo2 &&
                image.statusword == 0x0027U && near(image.actual_position, 1.5, 0.001) &&
                near(image.actual_velocity, -0.25, 0.001) && !short_frame.ok,
            "split classical-CAN RPDO/TPDO frames round trip with strict lengths"};
}

TestResult test_socketcan_canopen_backend_lifecycle_and_heartbeat() {
    const auto runtime = emrtc::load_runtime_adapter_config(
        "config/adapters/socketcan_canopen_single_axis.example.json");
    const auto& config = runtime.socketcan;
    FakeCanTransport transport;
    emrtc::SocketCanCanOpenBackend backend(transport);
    const auto configured = backend.configure(config);

    std::vector<emrtc::ProcessImage> images(1);
    images.front().controlword = 0x000FU;
    images.front().target_position = 1.0;
    images.front().target_velocity = 0.25;
    transport.incoming.push_back(make_heartbeat(config));
    transport.incoming.push_back(make_tpdo1(config, 0x0027U, 1000));
    transport.incoming.push_back(make_tpdo2(config, 250));
    const auto first_exchange = backend.exchange(0U, images);

    bool healthy_before_timeout = true;
    emrtc::BusExchangeResult timeout_exchange;
    for (emrtc::CycleIndex cycle = 1U; cycle <= config.heartbeat_timeout_cycles; ++cycle) {
        transport.incoming.push_back(make_tpdo1(config, 0x0027U, 1000));
        transport.incoming.push_back(make_tpdo2(config, 250));
        timeout_exchange = backend.exchange(cycle, images);
        if (cycle < config.heartbeat_timeout_cycles) {
            healthy_before_timeout = healthy_before_timeout && timeout_exchange.heartbeat_ok;
        }
    }

    const auto diagnostics = backend.diagnostics();
    const bool nmt_start = !transport.sent.empty() && transport.sent.front().id == 0U &&
                           transport.sent.front().dlc == 2U &&
                           transport.sent.front().data[0] == 0x01U &&
                           transport.sent.front().data[1] == config.node_id;
    return {"SocketCAN CANopen backend lifecycle and heartbeat",
            configured.ok && transport.opened_interface == "can0" && nmt_start &&
                first_exchange.ok && first_exchange.heartbeat_ok &&
                near(images.front().actual_position, 1.0, 0.001) &&
                near(images.front().actual_velocity, 0.25, 0.001) &&
                healthy_before_timeout && !timeout_exchange.ok &&
                !timeout_exchange.heartbeat_ok &&
                diagnostics.heartbeat_timeout_count == 1U &&
                diagnostics.online && !diagnostics.heartbeat_ok,
            "NMT, RPDO/TPDO exchange, scaling, and heartbeat timeout use an injected transport"};
}

TestResult test_socketcan_canopen_backend_reports_transport_loss() {
    const auto runtime = emrtc::load_runtime_adapter_config(
        "config/adapters/socketcan_canopen_single_axis.example.json");
    const auto& config = runtime.socketcan;
    FakeCanTransport transport;
    emrtc::SocketCanCanOpenBackend backend(transport);
    const auto configured = backend.configure(config);
    std::vector<emrtc::ProcessImage> images(1);
    transport.incoming.push_back(make_heartbeat(config));
    transport.incoming.push_back(make_tpdo1(config, 0x0027U, 0));
    transport.incoming.push_back(make_tpdo2(config, 0));
    const auto healthy = backend.exchange(0U, images);
    transport.fail_send = true;
    const auto failed = backend.exchange(1U, images);
    return {"SocketCAN CANopen backend reports transport loss",
            configured.ok && healthy.ok && !failed.ok &&
                !backend.diagnostics().online &&
                backend.diagnostics().dropout_count == 1U &&
                backend.diagnostics().cable_disconnect_events == 1U,
            "a transmit failure after a healthy cycle becomes an offline cable event"};
}

TestResult test_cia402_process_image_mapping_contract() {
    const auto& mapping = emrtc::cia402_process_image_mapping();
    const auto* controlword = emrtc::find_cia402_mapping("controlword");
    const auto* statusword = emrtc::find_cia402_mapping("statusword");
    const auto* target_position = emrtc::find_cia402_mapping("target_position");
    const bool object_map_ok = mapping.size() == 6U &&
                               controlword != nullptr &&
                               statusword != nullptr &&
                               target_position != nullptr &&
                               controlword->object_index == 0x6040 &&
                               controlword->controller_to_drive &&
                               statusword->object_index == 0x6041 &&
                               !statusword->controller_to_drive &&
                               target_position->object_index == 0x607A &&
                               target_position->bit_length == 32;

    emrtc::ProcessImage image;
    image.controlword = 0x000F;
    image.statusword = 0x0027;
    image.target_position = 1.25;
    image.target_velocity = 0.50;
    image.actual_position = 1.24;
    image.actual_velocity = 0.49;
    image.drive_fault = emrtc::FaultCode::None;

    const auto wire = emrtc::encode_cia402_wire_image(image, 1000.0);
    const auto decoded = emrtc::decode_cia402_wire_image(wire, 1000.0);
    const bool scale_ok = wire.target_position_counts == 1250 &&
                          wire.target_velocity_counts == 500 &&
                          wire.actual_position_counts == 1240 &&
                          wire.actual_velocity_counts == 490 &&
                          decoded.controlword == image.controlword &&
                          decoded.statusword == image.statusword &&
                          near(decoded.target_position, image.target_position, 0.001) &&
                          near(decoded.actual_velocity, image.actual_velocity, 0.001);

    return {"CiA 402 process-image mapping contract",
            object_map_ok && scale_ok,
            "object indexes and engineering-unit scaling round trip"};
}

TestResult test_adapter_mapping_validator_rejects_invalid_object_map() {
    auto valid = emrtc::expected_adapter_mapping();
    const auto valid_result = emrtc::validate_adapter_mapping(valid);
    valid[2].object_index = 0x6064;
    const auto invalid_result = emrtc::validate_adapter_mapping(valid);

    return {"adapter mapping validator rejects invalid object map",
            valid_result.ok &&
                !invalid_result.ok &&
                invalid_result.fault == emrtc::FaultCode::InvalidDriveTransition,
            "bad PDO/object mapping is rejected before hardware exchange"};
}

TestResult test_adapter_exchange_monitor_detects_stale_pdo_data() {
    emrtc::AdapterExchangeMonitor monitor(3);
    emrtc::BusDiagnostics diagnostics;
    diagnostics.rx_packets = 1;
    diagnostics.online = true;
    diagnostics.heartbeat_ok = true;

    emrtc::BusExchangeResult exchange;
    const auto initial = monitor.observe(0, exchange, diagnostics);
    const auto still_fresh = monitor.observe(2, exchange, diagnostics);
    const auto stale = monitor.observe(3, exchange, diagnostics);

    return {"adapter exchange monitor detects stale PDO data",
            initial.ok &&
                still_fresh.ok &&
                !stale.ok &&
                stale.fault == emrtc::FaultCode::BusDropout,
            "stale receive process image becomes a bus watchdog fault"};
}

TestResult test_adapter_exchange_monitor_detects_invalid_response() {
    emrtc::AdapterExchangeMonitor monitor;
    emrtc::BusDiagnostics diagnostics;
    diagnostics.rx_packets = 1;
    diagnostics.online = true;
    diagnostics.heartbeat_ok = true;

    emrtc::BusExchangeResult exchange;
    exchange.invalid_response = true;
    exchange.ok = false;
    const auto result = monitor.observe(10, exchange, diagnostics);

    return {"adapter exchange monitor detects invalid response",
            !result.ok && result.fault == emrtc::FaultCode::InvalidDriveTransition,
            "invalid PDO/object response is surfaced as an adapter contract fault"};
}

TestResult test_adapter_exchange_monitor_accepts_restored_exchange() {
    emrtc::AdapterExchangeMonitor monitor(3);
    emrtc::BusDiagnostics diagnostics;
    diagnostics.rx_packets = 1;
    diagnostics.online = true;
    diagnostics.heartbeat_ok = true;

    emrtc::BusExchangeResult exchange;
    const auto initial = monitor.observe(0, exchange, diagnostics);

    diagnostics.online = false;
    diagnostics.heartbeat_ok = false;
    exchange.ok = false;
    exchange.heartbeat_ok = false;
    const auto offline = monitor.observe(1, exchange, diagnostics);

    diagnostics.online = true;
    diagnostics.heartbeat_ok = true;
    diagnostics.rx_packets = 2;
    exchange.ok = true;
    exchange.heartbeat_ok = true;
    const auto restored = monitor.observe(4, exchange, diagnostics);

    return {"adapter exchange monitor accepts restored exchange",
            initial.ok &&
                !offline.ok &&
                offline.fault == emrtc::FaultCode::BusDropout &&
                restored.ok &&
                monitor.last_rx_cycle() == 4,
            "fresh receive data after bus restoration clears the adapter monitor observation"};
}

TestResult test_replay_adapter_nominal_exchange() {
    const auto summary = emrtc::run_replay_file("examples/replay/nominal_exchange.csv");
    return {"replay adapter nominal exchange",
            summary.pass &&
                summary.frames_processed == 5U &&
                summary.expectation_mismatches == 0U &&
                summary.contract_faults_observed == 0U &&
                summary.diagnostics.tx_packets == 5U &&
                summary.diagnostics.rx_packets == 5U &&
                summary.final_image.statusword == 39U &&
                near(summary.final_image.actual_position, 1.0, 0.001),
            "recorded process image reaches the expected final feedback"};
}

TestResult test_replay_adapter_fault_recovery_contract() {
    const auto summary = emrtc::run_replay_file(
        "examples/replay/fault_recovery_exchange.csv");
    return {"replay adapter fault and recovery contract",
            summary.pass &&
                summary.frames_processed == 11U &&
                summary.expectation_mismatches == 0U &&
                summary.contract_faults_observed == 4U &&
                summary.diagnostics.dropout_count == 1U &&
                summary.diagnostics.cable_disconnect_events == 1U &&
                summary.diagnostics.heartbeat_timeout_count == 1U &&
                summary.diagnostics.invalid_response_count == 1U &&
                summary.diagnostics.rx_packets == 5U &&
                near(summary.final_image.actual_position, 1.0, 0.001),
            "stale, offline, heartbeat, invalid-response, and restored frames match expectations"};
}

TestResult test_replay_adapter_rejects_controller_output_mismatch() {
    auto frames = emrtc::load_replay_frames_csv("examples/replay/nominal_exchange.csv");
    emrtc::ReplayBusBackend backend(std::move(frames));
    std::vector<emrtc::ProcessImage> images(1);
    images.front().controlword = 0x0006;
    images.front().target_position = 1.0;

    const auto exchange = backend.exchange(0, images);
    emrtc::AdapterExchangeMonitor monitor;
    const auto contract = monitor.observe(0, exchange, backend.diagnostics());
    return {"replay adapter rejects controller output mismatch",
            !exchange.ok &&
                exchange.invalid_response &&
                backend.diagnostics().invalid_response_count == 1U &&
                !contract.ok &&
                contract.fault == emrtc::FaultCode::InvalidDriveTransition,
            "wrong outgoing controlword is rejected before recorded feedback is applied"};
}

TestResult test_benchmark_motion_performance_metrics() {
    const auto metrics = emrtc::run_benchmark(2500);
    return {"benchmark motion performance metrics",
            metrics.motion.available &&
                metrics.motion.settled &&
                metrics.motion.overshoot <= 0.02 &&
                std::abs(metrics.motion.steady_state_error) <= 0.01 &&
                metrics.motion.settling_time_s > 0.0 &&
                metrics.two_axis.available &&
                metrics.two_axis.acceptance_pass &&
                metrics.two_axis.path_within_limit &&
                metrics.two_axis.final_position_error <= metrics.two_axis.target_tolerance &&
                metrics.recovery.available && metrics.recovery.fault_observed &&
                metrics.recovery.operation_reenabled &&
                metrics.recovery.post_recovery_motion_settled &&
                metrics.recovery.detection_latency_cycles <= 1U &&
                metrics.recovery.recovery_cycles <= 10U &&
                metrics.recovery.post_recovery_final_error <= 0.01 &&
                metrics.recovery.settling_time_ratio > 0.0 &&
                metrics.recovery.settling_time_ratio <= 1.5,
            "benchmark reports motion, two-axis, and quantitative recovery metrics"};
}

}  // namespace

int main() {
    const std::vector<TestResult> results = {
        test_normal_startup(),
        test_drive_enable_sequence(),
        test_absolute_move(),
        test_relative_move(),
        test_velocity_move(),
        test_quick_stop(),
        test_estop(),
        test_fault_reset_recovery(),
        test_invalid_transition(),
        test_missed_cycle(),
        test_cpu_load_jitter(),
        test_heartbeat_timeout(),
        test_bus_disconnect(),
        test_packet_loss(),
        test_encoder_loss(),
        test_following_error(),
        test_command_abuse(),
        test_sensor_noise(),
        test_warm_restart(),
        test_cold_restart(),
        test_fault_log_replay(),
        test_external_cia402_feedback_and_controlword_sequence(),
        test_controller_uses_injected_external_backend(),
        test_json_scenario_loader(),
        test_strict_json_parser(),
        test_runtime_adapter_config_validation(),
        test_runtime_adapter_config_rejects_unsafe_values(),
        test_host_realtime_probe_uses_absolute_schedule(),
        test_json_scenario_suite(),
        test_two_axis_coordinated_move(),
        test_hardware_adapter_skeletons_are_not_claiming_commissioning(),
        test_soem_configuration_guards_do_not_touch_hardware(),
        test_soem_pdo_codec_is_bounded_and_little_endian(),
        test_canopen_pdo_codec_is_bounded_and_little_endian(),
        test_socketcan_canopen_backend_lifecycle_and_heartbeat(),
        test_socketcan_canopen_backend_reports_transport_loss(),
        test_cia402_process_image_mapping_contract(),
        test_adapter_mapping_validator_rejects_invalid_object_map(),
        test_adapter_exchange_monitor_detects_stale_pdo_data(),
        test_adapter_exchange_monitor_detects_invalid_response(),
        test_adapter_exchange_monitor_accepts_restored_exchange(),
        test_replay_adapter_nominal_exchange(),
        test_replay_adapter_fault_recovery_contract(),
        test_replay_adapter_rejects_controller_output_mismatch(),
        test_benchmark_motion_performance_metrics(),
    };

    int failures = 0;
    for (const auto& result : results) {
        std::cout << (result.pass ? "PASS " : "FAIL ") << result.name
                  << " - " << result.detail << "\n";
        if (!result.pass) {
            failures++;
        }
    }

    std::cout << "tests=" << results.size() << " failures=" << failures << "\n";
    return failures == 0 ? 0 : 1;
}
