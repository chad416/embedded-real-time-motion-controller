#include "emrtc/axis.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>

namespace emrtc {
namespace {

constexpr double kPositionLoopGain = 16.0;

double clamp(double value, double low, double high) {
    return std::max(low, std::min(value, high));
}

const char* compact_state(DriveState state) {
    switch (state) {
        case DriveState::NotReady: return "not_ready";
        case DriveState::SwitchOnDisabled: return "switch_on_disabled";
        case DriveState::ReadyToSwitchOn: return "ready_to_switch_on";
        case DriveState::SwitchedOn: return "switched_on";
        case DriveState::OperationEnabled: return "operation_enabled";
        case DriveState::QuickStopActive: return "quick_stop_active";
        case DriveState::FaultReactionActive: return "fault_reaction_active";
        case DriveState::Fault: return "fault";
    }
    return "unknown";
}

}  // namespace

Axis::Axis(std::string name, AxisLimits limits)
    : name_(std::move(name)), limits_(limits) {}

const std::string& Axis::name() const {
    return name_;
}

DriveState Axis::state() const {
    return state_;
}

ControlMode Axis::mode() const {
    return mode_;
}

FaultCode Axis::fault_code() const {
    return fault_code_;
}

AxisFeedback Axis::feedback() const {
    AxisFeedback feedback;
    feedback.position = encoder_position_ + sensor_noise_;
    feedback.velocity = encoder_feedback_lost_ ? 0.0 : velocity_;
    feedback.target_position = target_position_;
    feedback.target_velocity = target_velocity_;
    feedback.following_error = (position_ - feedback.position) + injected_following_error_;
    feedback.statusword = build_statusword();
    feedback.controlword = build_controlword();
    feedback.fault_code = fault_code_;
    feedback.state = state_;
    feedback.mode = mode_;
    feedback.target_reached = target_reached_;
    feedback.encoder_feedback_lost = encoder_feedback_lost_;
    return feedback;
}

bool Axis::apply_command(const AxisCommand& command, CycleIndex cycle) {
    switch (command.type) {
        case CommandType::None:
            return true;
        case CommandType::Enable:
            if (state_ == DriveState::Fault || state_ == DriveState::FaultReactionActive) {
                enter_fault(FaultCode::CommandRejected, cycle, "enable rejected while faulted");
                return false;
            }
            enable_requested_ = true;
            return true;
        case CommandType::Disable:
            enable_requested_ = false;
            mode_ = ControlMode::Disabled;
            target_velocity_ = 0.0;
            set_state(DriveState::SwitchOnDisabled, cycle, "disable command");
            return true;
        case CommandType::FaultReset:
            if (state_ == DriveState::Fault) {
                fault_code_ = FaultCode::None;
                encoder_feedback_lost_ = false;
                injected_following_error_ = 0.0;
                sensor_noise_ = 0.0;
                set_state(DriveState::SwitchOnDisabled, cycle, "fault reset");
                return true;
            }
            return true;
        case CommandType::QuickStop:
        case CommandType::SafeStop:
            target_velocity_ = 0.0;
            mode_ = ControlMode::ProfileVelocity;
            set_state(DriveState::QuickStopActive, cycle, command.type == CommandType::SafeStop ? "safe stop" : "quick stop");
            if (command.type == CommandType::SafeStop) {
                fault_code_ = FaultCode::EmergencyStop;
            }
            return true;
        case CommandType::MoveAbsolute:
            if (state_ != DriveState::OperationEnabled) {
                enter_fault(FaultCode::CommandRejected, cycle, "absolute move rejected unless operation enabled");
                return false;
            }
            mode_ = ControlMode::ProfilePosition;
            target_position_ = command.value;
            target_reached_ = false;
            return true;
        case CommandType::MoveRelative:
            if (state_ != DriveState::OperationEnabled) {
                enter_fault(FaultCode::CommandRejected, cycle, "relative move rejected unless operation enabled");
                return false;
            }
            mode_ = ControlMode::ProfilePosition;
            target_position_ = position_ + command.value;
            target_reached_ = false;
            return true;
        case CommandType::Velocity:
            if (state_ != DriveState::OperationEnabled) {
                enter_fault(FaultCode::CommandRejected, cycle, "velocity command rejected unless operation enabled");
                return false;
            }
            mode_ = ControlMode::ProfileVelocity;
            target_velocity_ = clamp(command.value,
                                     -limits_.max_velocity_units_per_s,
                                     limits_.max_velocity_units_per_s);
            target_reached_ = false;
            return true;
        case CommandType::Stop:
            target_velocity_ = 0.0;
            target_position_ = position_;
            target_reached_ = true;
            mode_ = ControlMode::ProfileVelocity;
            return true;
    }
    return false;
}

void Axis::update(double dt_s, CycleIndex cycle) {
    if (!bus_online_ && state_ != DriveState::Fault && state_ != DriveState::FaultReactionActive) {
        enter_fault(FaultCode::BusDropout, cycle, "bus offline");
    }

    if (state_ == DriveState::NotReady) {
        set_state(DriveState::SwitchOnDisabled, cycle, "boot complete");
    } else if (enable_requested_ && state_ == DriveState::SwitchOnDisabled) {
        set_state(DriveState::ReadyToSwitchOn, cycle, "cia402 shutdown accepted");
    } else if (enable_requested_ && state_ == DriveState::ReadyToSwitchOn) {
        set_state(DriveState::SwitchedOn, cycle, "cia402 switch on accepted");
    } else if (enable_requested_ && state_ == DriveState::SwitchedOn) {
        set_state(DriveState::OperationEnabled, cycle, "cia402 enable operation accepted");
    } else if (state_ == DriveState::FaultReactionActive) {
        set_state(DriveState::Fault, cycle, "fault reaction completed");
    }

    update_motion(dt_s);

    if (!encoder_feedback_lost_) {
        encoder_position_ = position_;
    }
}

void Axis::set_bus_online(bool online) {
    bus_online_ = online;
}

void Axis::request_safe_stop(CycleIndex cycle) {
    apply_command({CommandType::SafeStop, 0.0}, cycle);
}

void Axis::inject_fault(FaultKind kind, double magnitude, CycleIndex cycle) {
    switch (kind) {
        case FaultKind::InvalidDriveTransition:
            enter_fault(FaultCode::InvalidDriveTransition, cycle, "forced invalid CiA 402 transition");
            break;
        case FaultKind::EncoderFeedbackLoss:
            encoder_feedback_lost_ = true;
            break;
        case FaultKind::SensorNoise:
            sensor_noise_ = magnitude;
            break;
        case FaultKind::FollowingError:
            injected_following_error_ = magnitude == 0.0 ? limits_.following_error_limit * 2.0 : magnitude;
            break;
        case FaultKind::DriveFault:
            enter_fault(FaultCode::DriveFault, cycle, "simulated drive fault");
            break;
        case FaultKind::CommandAbuse:
            enter_fault(FaultCode::CommandRejected, cycle, "invalid command during motion");
            break;
        case FaultKind::MissedCycle:
        case FaultKind::CpuLoadSpike:
        case FaultKind::HeartbeatTimeout:
        case FaultKind::BusDropout:
        case FaultKind::PacketLoss:
        case FaultKind::WatchdogTimeout:
            break;
    }
}

void Axis::force_fault(FaultCode code, CycleIndex cycle, const std::string& reason) {
    enter_fault(code, cycle, reason);
}

void Axis::clear_transient_faults() {
    sensor_noise_ = 0.0;
    injected_following_error_ = 0.0;
    encoder_feedback_lost_ = false;
}

void Axis::cold_restart() {
    state_ = DriveState::NotReady;
    mode_ = ControlMode::Disabled;
    fault_code_ = FaultCode::None;
    enable_requested_ = false;
    bus_online_ = true;
    encoder_feedback_lost_ = false;
    sensor_noise_ = 0.0;
    injected_following_error_ = 0.0;
    position_ = 0.0;
    encoder_position_ = 0.0;
    velocity_ = 0.0;
    target_position_ = 0.0;
    target_velocity_ = 0.0;
    target_reached_ = true;
    transition_trace_.clear();
}

void Axis::warm_restart() {
    state_ = DriveState::SwitchOnDisabled;
    mode_ = ControlMode::Disabled;
    fault_code_ = FaultCode::None;
    enable_requested_ = false;
    velocity_ = 0.0;
    target_velocity_ = 0.0;
    target_position_ = encoder_position_;
    position_ = encoder_position_;
    target_reached_ = true;
    clear_transient_faults();
}

const std::vector<std::string>& Axis::transition_trace() const {
    return transition_trace_;
}

void Axis::set_state(DriveState next, CycleIndex cycle, const std::string& reason) {
    if (state_ == next) {
        return;
    }

    std::ostringstream entry;
    entry << "cycle=" << cycle << " axis=" << name_ << " "
          << compact_state(state_) << "->" << compact_state(next)
          << " reason=" << reason;
    transition_trace_.push_back(entry.str());
    state_ = next;
}

void Axis::enter_fault(FaultCode code, CycleIndex cycle, const std::string& reason) {
    fault_code_ = code;
    enable_requested_ = false;
    mode_ = ControlMode::Disabled;
    target_velocity_ = 0.0;
    set_state(DriveState::FaultReactionActive, cycle, reason);
}

std::uint16_t Axis::build_statusword() const {
    switch (state_) {
        case DriveState::NotReady: return 0x0000;
        case DriveState::SwitchOnDisabled: return 0x0040;
        case DriveState::ReadyToSwitchOn: return 0x0021;
        case DriveState::SwitchedOn: return 0x0023;
        case DriveState::OperationEnabled: return 0x0027;
        case DriveState::QuickStopActive: return 0x0007;
        case DriveState::FaultReactionActive: return 0x000F;
        case DriveState::Fault: return 0x0008;
    }
    return 0x0000;
}

std::uint16_t Axis::build_controlword() const {
    if (state_ == DriveState::OperationEnabled) {
        return 0x000F;
    }
    if (enable_requested_) {
        return 0x0006;
    }
    if (state_ == DriveState::QuickStopActive) {
        return 0x0002;
    }
    return 0x0000;
}

void Axis::update_motion(double dt_s) {
    if (state_ != DriveState::OperationEnabled && state_ != DriveState::QuickStopActive) {
        velocity_ = 0.0;
        return;
    }

    if (mode_ == ControlMode::ProfilePosition || mode_ == ControlMode::CyclicSynchronousPosition) {
        const double position_error = target_position_ - position_;
        const double desired_velocity = clamp(kPositionLoopGain * position_error,
                                              -limits_.max_velocity_units_per_s,
                                              limits_.max_velocity_units_per_s);
        const double accel_step = limits_.max_accel_units_per_s2 * dt_s;
        velocity_ += clamp(desired_velocity - velocity_, -accel_step, accel_step);
        position_ += velocity_ * dt_s;
        if (std::abs(position_error) < 0.0005 && std::abs(velocity_) < 0.002) {
            position_ = target_position_;
            velocity_ = 0.0;
            target_reached_ = true;
        }
    } else if (mode_ == ControlMode::ProfileVelocity || mode_ == ControlMode::CyclicSynchronousVelocity) {
        const double accel_step = limits_.max_accel_units_per_s2 * dt_s;
        velocity_ += clamp(target_velocity_ - velocity_, -accel_step, accel_step);
        position_ += velocity_ * dt_s;
        target_reached_ = std::abs(velocity_) < 0.002 && std::abs(target_velocity_) < 0.002;
    }

    if (state_ == DriveState::QuickStopActive && std::abs(velocity_) < 0.002) {
        velocity_ = 0.0;
        target_reached_ = true;
    }
}

}  // namespace emrtc
