#include "emrtc/adapter_contract.hpp"
#include "emrtc/controller.hpp"
#include "emrtc/hardware_adapters.hpp"
#include "emrtc/multi_axis.hpp"
#include "emrtc/process_image_mapping.hpp"
#include "emrtc/replay_adapter.hpp"
#include "emrtc/scenario.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct TestResult {
    std::string name;
    bool pass{false};
    std::string detail;
};

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
                metrics.two_axis.final_position_error <= metrics.two_axis.target_tolerance,
            "benchmark reports overshoot, settling, steady-state, and two-axis acceptance metrics"};
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
        test_json_scenario_loader(),
        test_json_scenario_suite(),
        test_two_axis_coordinated_move(),
        test_hardware_adapter_skeletons_are_not_claiming_commissioning(),
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
