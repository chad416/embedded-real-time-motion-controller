#include "emrtc/types.hpp"

namespace emrtc {

std::string to_string(DriveState value) {
    switch (value) {
        case DriveState::NotReady: return "Not Ready";
        case DriveState::SwitchOnDisabled: return "Switch On Disabled";
        case DriveState::ReadyToSwitchOn: return "Ready To Switch On";
        case DriveState::SwitchedOn: return "Switched On";
        case DriveState::OperationEnabled: return "Operation Enabled";
        case DriveState::QuickStopActive: return "Quick Stop Active";
        case DriveState::FaultReactionActive: return "Fault Reaction Active";
        case DriveState::Fault: return "Fault";
    }
    return "Unknown";
}

std::string to_string(ControlMode value) {
    switch (value) {
        case ControlMode::Disabled: return "Disabled";
        case ControlMode::ProfilePosition: return "Profile Position";
        case ControlMode::ProfileVelocity: return "Profile Velocity";
        case ControlMode::CyclicSynchronousPosition: return "Cyclic Synchronous Position";
        case ControlMode::CyclicSynchronousVelocity: return "Cyclic Synchronous Velocity";
    }
    return "Unknown";
}

std::string to_string(CommandType value) {
    switch (value) {
        case CommandType::None: return "none";
        case CommandType::Enable: return "enable";
        case CommandType::Disable: return "disable";
        case CommandType::FaultReset: return "fault_reset";
        case CommandType::QuickStop: return "quick_stop";
        case CommandType::MoveAbsolute: return "move_absolute";
        case CommandType::MoveRelative: return "move_relative";
        case CommandType::Velocity: return "velocity";
        case CommandType::Stop: return "stop";
        case CommandType::SafeStop: return "safe_stop";
    }
    return "unknown";
}

std::string to_string(FaultCode value) {
    switch (value) {
        case FaultCode::None: return "None";
        case FaultCode::MissedCycle: return "Missed Cycle";
        case FaultCode::DeadlineOverrun: return "Deadline Overrun";
        case FaultCode::HeartbeatTimeout: return "Heartbeat Timeout";
        case FaultCode::BusDropout: return "Bus Dropout";
        case FaultCode::PacketLoss: return "Packet Loss";
        case FaultCode::InvalidDriveTransition: return "Invalid Drive Transition";
        case FaultCode::EncoderFeedbackLoss: return "Encoder Feedback Loss";
        case FaultCode::FollowingError: return "Following Error";
        case FaultCode::DriveFault: return "Drive Fault";
        case FaultCode::WatchdogTimeout: return "Watchdog Timeout";
        case FaultCode::CommandRejected: return "Command Rejected";
        case FaultCode::EmergencyStop: return "Emergency Stop";
    }
    return "Unknown";
}

std::string to_string(FaultKind value) {
    switch (value) {
        case FaultKind::MissedCycle: return "missed_cycle";
        case FaultKind::CpuLoadSpike: return "cpu_load_spike";
        case FaultKind::HeartbeatTimeout: return "heartbeat_timeout";
        case FaultKind::BusDropout: return "bus_dropout";
        case FaultKind::PacketLoss: return "packet_loss";
        case FaultKind::InvalidDriveTransition: return "invalid_drive_transition";
        case FaultKind::EncoderFeedbackLoss: return "encoder_feedback_loss";
        case FaultKind::SensorNoise: return "sensor_noise";
        case FaultKind::FollowingError: return "following_error";
        case FaultKind::DriveFault: return "drive_fault";
        case FaultKind::WatchdogTimeout: return "watchdog_timeout";
        case FaultKind::CommandAbuse: return "command_abuse";
    }
    return "unknown";
}

std::string to_string(ScenarioResult value) {
    switch (value) {
        case ScenarioResult::Pass: return "PASS";
        case ScenarioResult::Fail: return "FAIL";
    }
    return "UNKNOWN";
}

}  // namespace emrtc
