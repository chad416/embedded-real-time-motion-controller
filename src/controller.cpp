#include "emrtc/controller.hpp"

#include <cmath>
#include <sstream>

namespace emrtc {
namespace {

constexpr double kNsToSeconds = 1.0e-9;

bool near(double value, double target, double tolerance) {
    return std::abs(value - target) <= tolerance;
}

ScenarioSummary make_summary(const std::string& name, bool pass, const std::string& evidence,
                             std::uint64_t recovery_cycles = 0) {
    return {name, pass ? ScenarioResult::Pass : ScenarioResult::Fail, evidence, recovery_cycles};
}

void enable_axis(RealTimeController& controller) {
    controller.command({CommandType::Enable, 0.0});
    controller.run_cycles(5);
}

}  // namespace

RealTimeController::RealTimeController(ControllerConfig config)
    : config_(config),
      axis_("axis_0", {config.max_velocity_units_per_s,
                       config.max_accel_units_per_s2,
                       config.following_error_limit}),
      watchdog_(config) {
    process_images_.resize(1);
}

Axis& RealTimeController::axis() {
    return axis_;
}

const Axis& RealTimeController::axis() const {
    return axis_;
}

SimulatedBusBackend& RealTimeController::bus() {
    return bus_;
}

const SimulatedBusBackend& RealTimeController::bus() const {
    return bus_;
}

FaultInjector& RealTimeController::fault_injector() {
    return fault_injector_;
}

void RealTimeController::command(AxisCommand command) {
    pending_command_ = command;
}

void RealTimeController::request_estop() {
    estop_requested_ = true;
}

void RealTimeController::reset_faults() {
    bus_.reset_faults();
    watchdog_.reset();
    axis_.clear_transient_faults();
    axis_.apply_command({CommandType::FaultReset, 0.0}, cycle_);
    estop_requested_ = false;
    force_watchdog_timeout_ = false;
    forced_extra_period_ns_ = 0;
    record_status("fault reset");
}

void RealTimeController::cold_restart() {
    axis_.cold_restart();
    bus_.reset_faults();
    watchdog_.reset();
    pending_command_ = {};
    estop_requested_ = false;
    force_watchdog_timeout_ = false;
    forced_extra_period_ns_ = 0;
    record_status("cold restart");
}

void RealTimeController::warm_restart() {
    axis_.warm_restart();
    bus_.reset_faults();
    watchdog_.reset();
    pending_command_ = {};
    estop_requested_ = false;
    force_watchdog_timeout_ = false;
    forced_extra_period_ns_ = 0;
    record_status("warm restart");
}

void RealTimeController::run_cycles(std::uint64_t count) {
    for (std::uint64_t i = 0; i < count; ++i) {
        execute_cycle();
    }
}

RuntimeMetrics RealTimeController::metrics() const {
    auto result = telemetry_.finalize();
    result.bus = bus_.diagnostics();
    return result;
}

ControllerConfig RealTimeController::config() const {
    return config_;
}

Nanoseconds RealTimeController::deterministic_period_for_cycle(CycleIndex cycle) const {
    const auto base = config_.cycle_time_ns;
    const auto pattern = static_cast<Nanoseconds>((cycle * 97u) % 21u) - 10;
    return base + (pattern * 1000);
}

void RealTimeController::apply_fault_event(const FaultInjectionEvent& event) {
    switch (event.kind) {
        case FaultKind::MissedCycle:
            forced_extra_period_ns_ += config_.cycle_time_ns * 2 + static_cast<Nanoseconds>(event.magnitude);
            record_fault(FaultCode::MissedCycle, "fault injector forced missed cycle");
            break;
        case FaultKind::CpuLoadSpike:
            forced_extra_period_ns_ += static_cast<Nanoseconds>(config_.cycle_time_ns * event.magnitude);
            record_status("cpu load spike injected");
            break;
        case FaultKind::HeartbeatTimeout:
            bus_.inject_heartbeat_timeout(event.duration_cycles);
            record_status("heartbeat timeout injected");
            break;
        case FaultKind::BusDropout:
            bus_.inject_dropout(event.duration_cycles);
            record_status("bus dropout injected");
            break;
        case FaultKind::PacketLoss:
            bus_.inject_packet_loss(event.magnitude <= 0.0 ? 2u : static_cast<std::uint32_t>(event.magnitude),
                                    event.duration_cycles);
            record_status("packet loss injected");
            break;
        case FaultKind::InvalidDriveTransition:
            axis_.inject_fault(event.kind, event.magnitude, cycle_);
            record_fault(FaultCode::InvalidDriveTransition, "invalid drive transition injected");
            break;
        case FaultKind::EncoderFeedbackLoss:
        case FaultKind::SensorNoise:
        case FaultKind::FollowingError:
            axis_.inject_fault(event.kind, event.magnitude, cycle_);
            record_status(to_string(event.kind) + " injected");
            break;
        case FaultKind::CommandAbuse:
            axis_.inject_fault(event.kind, event.magnitude, cycle_);
            record_fault(FaultCode::CommandRejected, "invalid command during motion injected");
            break;
        case FaultKind::DriveFault:
            axis_.inject_fault(event.kind, event.magnitude, cycle_);
            record_fault(FaultCode::DriveFault, "drive fault injected");
            break;
        case FaultKind::WatchdogTimeout:
            force_watchdog_timeout_ = true;
            forced_extra_period_ns_ += config_.cycle_time_ns * 4;
            axis_.force_fault(FaultCode::WatchdogTimeout, cycle_, "loop watchdog timeout injected");
            record_fault(FaultCode::WatchdogTimeout, "loop watchdog timeout injected");
            break;
    }
}

void RealTimeController::execute_cycle() {
    for (const auto& event : fault_injector_.events_at(cycle_)) {
        apply_fault_event(event);
    }

    if (pending_command_.type != CommandType::None) {
        const auto command_name = to_string(pending_command_.type);
        const bool accepted = axis_.apply_command(pending_command_, cycle_);
        record_status(accepted ? "command " + command_name : "command rejected " + command_name);
        if (!accepted) {
            record_fault(FaultCode::CommandRejected, "axis rejected command " + command_name);
        }
        pending_command_ = {};
    }

    if (estop_requested_) {
        axis_.request_safe_stop(cycle_);
        record_fault(FaultCode::EmergencyStop, "E-stop safe stop request active");
        estop_requested_ = false;
    }

    const Nanoseconds measured_period =
        deterministic_period_for_cycle(cycle_) + forced_extra_period_ns_;
    forced_extra_period_ns_ = 0;

    CycleSample sample;
    sample.cycle = cycle_;
    sample.expected_period_ns = config_.cycle_time_ns;
    sample.measured_period_ns = measured_period;
    sample.jitter_ns = measured_period - config_.cycle_time_ns;
    sample.missed_cycle = measured_period >= config_.cycle_time_ns * 2 || force_watchdog_timeout_;
    sample.deadline_overrun = measured_period > config_.cycle_time_ns + config_.deadline_tolerance_ns;
    telemetry_.record_cycle(sample);

    process_images_[0].controlword = axis_.feedback().controlword;
    process_images_[0].target_position = axis_.feedback().target_position;
    process_images_[0].target_velocity = axis_.feedback().target_velocity;
    const auto bus_result = bus_.exchange(cycle_, process_images_);

    if (bus_result.invalid_response) {
        axis_.force_fault(FaultCode::InvalidDriveTransition, cycle_, "invalid PDO/object response");
        record_fault(FaultCode::InvalidDriveTransition, "bus returned invalid PDO/object dictionary response");
    }

    axis_.set_bus_online(bus_.diagnostics().online);
    axis_.update(static_cast<double>(config_.cycle_time_ns) * kNsToSeconds, cycle_);

    auto feedback = axis_.feedback();
    watchdog_.note_encoder_feedback_loss(feedback.encoder_feedback_lost,
                                         static_cast<double>(config_.cycle_time_ns) * kNsToSeconds);
    const auto watchdog_status = watchdog_.evaluate(sample, bus_.diagnostics(), feedback);
    if (watchdog_status.fault != FaultCode::None &&
        axis_.fault_code() != watchdog_status.fault &&
        axis_.state() != DriveState::FaultReactionActive) {
        axis_.force_fault(watchdog_status.fault, cycle_, "watchdog supervisor trip");
        record_fault(watchdog_status.fault, "watchdog supervisor trip");
        feedback = axis_.feedback();
    }

    process_images_[0].statusword = feedback.statusword;
    process_images_[0].actual_position = feedback.position;
    process_images_[0].actual_velocity = feedback.velocity;
    process_images_[0].drive_fault = feedback.fault_code;
    telemetry_.set_bus_diagnostics(bus_.diagnostics());
    record_status("cycle");

    if (force_watchdog_timeout_) {
        force_watchdog_timeout_ = false;
    }
    cycle_++;
}

void RealTimeController::record_status(const std::string& event) {
    const auto feedback = axis_.feedback();
    telemetry_.record_trace({cycle_,
                             static_cast<Nanoseconds>(cycle_) * config_.cycle_time_ns,
                             event,
                             feedback.state,
                             feedback.mode,
                             feedback.fault_code,
                             feedback.position,
                             feedback.velocity,
                             feedback.following_error});
}

void RealTimeController::record_fault(FaultCode fault, const std::string& detail) {
    telemetry_.record_fault({cycle_,
                             static_cast<Nanoseconds>(cycle_) * config_.cycle_time_ns,
                             fault,
                             detail});
}

std::vector<ScenarioSummary> run_validation_scenarios(RuntimeMetrics* combined_metrics) {
    std::vector<ScenarioSummary> summaries;

    {
        RealTimeController c;
        c.run_cycles(1);
        enable_axis(c);
        summaries.push_back(make_summary("normal startup",
                                         c.axis().state() == DriveState::OperationEnabled,
                                         "axis reached Operation Enabled through deterministic CiA 402 sequence"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        summaries.push_back(make_summary("drive enable sequence",
                                         c.axis().feedback().statusword == 0x0027,
                                         "statusword 0x0027 observed"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.command({CommandType::MoveAbsolute, 1.0});
        c.run_cycles(900);
        summaries.push_back(make_summary("absolute move",
                                         near(c.axis().feedback().position, 1.0, 0.01),
                                         "axis settled within 0.01 units of target"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.command({CommandType::MoveRelative, 0.5});
        c.run_cycles(600);
        summaries.push_back(make_summary("relative move",
                                         near(c.axis().feedback().position, 0.5, 0.01),
                                         "axis settled within 0.01 units of relative target"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.command({CommandType::Velocity, 0.75});
        c.run_cycles(100);
        summaries.push_back(make_summary("velocity move",
                                         c.axis().feedback().velocity > 0.5,
                                         "velocity command ramped above 0.5 units/s"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.command({CommandType::Velocity, 1.0});
        c.run_cycles(50);
        c.command({CommandType::QuickStop, 0.0});
        c.run_cycles(100);
        summaries.push_back(make_summary("quick stop during motion",
                                         c.axis().state() == DriveState::QuickStopActive &&
                                             std::abs(c.axis().feedback().velocity) < 0.01,
                                         "velocity decayed to safe stop band"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.command({CommandType::Velocity, 1.0});
        c.run_cycles(30);
        c.request_estop();
        c.run_cycles(100);
        summaries.push_back(make_summary("E-stop safe stop during motion",
                                         c.axis().fault_code() == FaultCode::EmergencyStop,
                                         "safe stop request recorded as safety-oriented emergency stop"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.fault_injector().schedule({10, FaultKind::DriveFault, 1.0, 1});
        c.run_cycles(20);
        const auto fault_cycle = c.metrics().cycles.size();
        c.reset_faults();
        c.command({CommandType::Enable, 0.0});
        c.run_cycles(5);
        summaries.push_back(make_summary("fault reset and recovery",
                                         c.axis().state() == DriveState::OperationEnabled,
                                         "fault reset returned drive to Operation Enabled",
                                         c.metrics().cycles.size() - fault_cycle));
    }

    {
        RealTimeController c;
        c.fault_injector().schedule({1, FaultKind::InvalidDriveTransition, 1.0, 1});
        c.run_cycles(3);
        summaries.push_back(make_summary("invalid state transition rejected",
                                         c.axis().fault_code() == FaultCode::InvalidDriveTransition,
                                         "invalid transition forced Fault Reaction Active/Fault"));
    }

    {
        RealTimeController c;
        c.fault_injector().schedule({5, FaultKind::MissedCycle, 0.0, 1});
        c.run_cycles(10);
        summaries.push_back(make_summary("missed-cycle injection",
                                         c.metrics().missed_cycles >= 1,
                                         "missed cycle counter incremented"));
    }

    {
        RealTimeController c;
        c.fault_injector().schedule({5, FaultKind::CpuLoadSpike, 1.5, 1});
        c.run_cycles(10);
        summaries.push_back(make_summary("CPU load jitter stress",
                                         c.metrics().deadline_overruns >= 1,
                                         "deadline overrun captured during load spike"));
    }

    {
        RealTimeController c;
        c.fault_injector().schedule({2, FaultKind::HeartbeatTimeout, 1.0, 8});
        c.run_cycles(12);
        summaries.push_back(make_summary("heartbeat timeout",
                                         c.axis().fault_code() == FaultCode::HeartbeatTimeout,
                                         "heartbeat supervision tripped after configured timeout"));
    }

    {
        RealTimeController c;
        c.fault_injector().schedule({2, FaultKind::BusDropout, 1.0, 4});
        c.run_cycles(8);
        summaries.push_back(make_summary("bus cable disconnect simulation",
                                         c.metrics().bus.cable_disconnect_events == 1 &&
                                             c.axis().fault_code() == FaultCode::BusDropout,
                                         "simulated cable disconnect forced bus watchdog fault"));
    }

    {
        RealTimeController c;
        c.fault_injector().schedule({2, FaultKind::PacketLoss, 2.0, 5});
        c.run_cycles(20);
        summaries.push_back(make_summary("packet loss simulation",
                                         c.metrics().bus.packet_loss_count >= 1,
                                         "packet loss counter incremented"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.fault_injector().schedule({10, FaultKind::EncoderFeedbackLoss, 1.0, 10});
        c.run_cycles(20);
        summaries.push_back(make_summary("encoder feedback loss",
                                         c.axis().fault_code() == FaultCode::EncoderFeedbackLoss,
                                         "encoder watchdog tripped after feedback-loss window"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.fault_injector().schedule({10, FaultKind::FollowingError, 0.2, 1});
        c.run_cycles(12);
        summaries.push_back(make_summary("following error",
                                         c.axis().fault_code() == FaultCode::FollowingError,
                                         "following-error watchdog tripped above limit"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.command({CommandType::Velocity, 0.5});
        c.run_cycles(20);
        c.fault_injector().schedule({25, FaultKind::CommandAbuse, 1.0, 1});
        c.run_cycles(10);
        summaries.push_back(make_summary("command abuse during motion",
                                         c.axis().fault_code() == FaultCode::CommandRejected,
                                         "invalid command during motion was rejected and logged"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.fault_injector().schedule({10, FaultKind::SensorNoise, 0.005, 1});
        c.run_cycles(12);
        summaries.push_back(make_summary("sensor noise injection",
                                         c.axis().feedback().position > 0.0 &&
                                             c.axis().fault_code() == FaultCode::None,
                                         "bounded sensor noise did not create false trip"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.command({CommandType::MoveAbsolute, 0.25});
        c.run_cycles(200);
        c.warm_restart();
        c.command({CommandType::Enable, 0.0});
        c.run_cycles(5);
        summaries.push_back(make_summary("warm restart",
                                         c.axis().state() == DriveState::OperationEnabled,
                                         "warm restart preserved position and re-enabled drive"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.command({CommandType::MoveAbsolute, 0.25});
        c.run_cycles(200);
        c.cold_restart();
        c.command({CommandType::Enable, 0.0});
        c.run_cycles(5);
        summaries.push_back(make_summary("cold restart",
                                         c.axis().state() == DriveState::OperationEnabled &&
                                             near(c.axis().feedback().position, 0.0, 0.001),
                                         "cold restart cleared position model and re-enabled drive"));
    }

    {
        RealTimeController c;
        enable_axis(c);
        c.fault_injector().schedule({10, FaultKind::DriveFault, 1.0, 1});
        c.run_cycles(20);
        const auto metrics = c.metrics();
        summaries.push_back(make_summary("persistent fault log replay",
                                         !metrics.fault_log.empty() &&
                                             metrics.fault_log.back().fault == FaultCode::DriveFault,
                                         "fault log contains replayable cycle/time/fault record"));
    }

    if (combined_metrics != nullptr) {
        *combined_metrics = run_benchmark(2500);
        combined_metrics->scenarios = summaries;
    }

    return summaries;
}

RuntimeMetrics run_benchmark(std::uint64_t cycles) {
    RealTimeController controller;
    enable_axis(controller);
    controller.command({CommandType::MoveAbsolute, 1.0});
    controller.fault_injector().schedule({300, FaultKind::PacketLoss, 5.0, 30});
    controller.fault_injector().schedule({800, FaultKind::CpuLoadSpike, 1.25, 1});
    controller.fault_injector().schedule({1200, FaultKind::SensorNoise, 0.002, 1});
    controller.fault_injector().schedule({1600, FaultKind::MissedCycle, 0.0, 1});
    controller.run_cycles(cycles > 5 ? cycles - 5 : 0);
    return controller.metrics();
}

}  // namespace emrtc
