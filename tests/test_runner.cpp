#include "emrtc/controller.hpp"
#include "emrtc/hardware_adapters.hpp"
#include "emrtc/multi_axis.hpp"
#include "emrtc/scenario.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
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

TestResult test_two_axis_coordinated_move() {
    const auto metrics = emrtc::simulate_two_axis_linear_move();
    return {"two-axis coordinated move",
            metrics.target_reached && metrics.path_within_limit &&
                near(metrics.final_x, 1.0, 0.01) &&
                near(metrics.final_y, 1.0, 0.01),
            "two simulated axes settle on a bounded straight-line path"};
}

TestResult test_hardware_adapter_skeletons_are_not_claiming_commissioning() {
    emrtc::SoemEtherCatBackend soem;
    emrtc::SocketCanCanOpenBackend socket_can;
    const auto soem_status = soem.status();
    const auto can_status = socket_can.status();
    return {"hardware adapter skeleton status",
            !soem_status.configured && !can_status.configured &&
                soem.diagnostics().online == false &&
                socket_can.diagnostics().online == false,
            "future hardware adapters report uncommissioned status"};
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
        test_two_axis_coordinated_move(),
        test_hardware_adapter_skeletons_are_not_claiming_commissioning(),
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
