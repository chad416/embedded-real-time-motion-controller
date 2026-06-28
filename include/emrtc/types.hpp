#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace emrtc {

using CycleIndex = std::uint64_t;
using Nanoseconds = std::int64_t;

enum class DriveState {
    NotReady,
    SwitchOnDisabled,
    ReadyToSwitchOn,
    SwitchedOn,
    OperationEnabled,
    QuickStopActive,
    FaultReactionActive,
    Fault
};

enum class ControlMode {
    Disabled,
    ProfilePosition,
    ProfileVelocity,
    CyclicSynchronousPosition,
    CyclicSynchronousVelocity
};

enum class CommandType {
    None,
    Enable,
    Disable,
    FaultReset,
    QuickStop,
    MoveAbsolute,
    MoveRelative,
    Velocity,
    Stop,
    SafeStop
};

enum class FaultCode {
    None,
    MissedCycle,
    DeadlineOverrun,
    HeartbeatTimeout,
    BusDropout,
    PacketLoss,
    InvalidDriveTransition,
    EncoderFeedbackLoss,
    FollowingError,
    DriveFault,
    WatchdogTimeout,
    CommandRejected,
    EmergencyStop
};

enum class FaultKind {
    MissedCycle,
    CpuLoadSpike,
    HeartbeatTimeout,
    BusDropout,
    PacketLoss,
    InvalidDriveTransition,
    EncoderFeedbackLoss,
    SensorNoise,
    FollowingError,
    DriveFault,
    WatchdogTimeout,
    CommandAbuse
};

enum class ScenarioResult {
    Pass,
    Fail
};

struct ControllerConfig {
    Nanoseconds cycle_time_ns{1'000'000};
    Nanoseconds deadline_tolerance_ns{100'000};
    std::uint32_t loop_watchdog_limit_cycles{3};
    std::uint32_t heartbeat_timeout_cycles{5};
    double following_error_limit{0.050};
    double encoder_loss_timeout_s{0.004};
    double max_velocity_units_per_s{2.0};
    double max_accel_units_per_s2{40.0};
};

struct AxisCommand {
    CommandType type{CommandType::None};
    double value{0.0};
};

struct AxisFeedback {
    double position{0.0};
    double velocity{0.0};
    double target_position{0.0};
    double target_velocity{0.0};
    double following_error{0.0};
    std::uint16_t statusword{0};
    std::uint16_t controlword{0};
    FaultCode fault_code{FaultCode::None};
    DriveState state{DriveState::NotReady};
    ControlMode mode{ControlMode::Disabled};
    bool target_reached{true};
    bool encoder_feedback_lost{false};
};

struct CycleSample {
    CycleIndex cycle{0};
    Nanoseconds expected_period_ns{0};
    Nanoseconds measured_period_ns{0};
    Nanoseconds jitter_ns{0};
    bool missed_cycle{false};
    bool deadline_overrun{false};
};

struct ScenarioSummary {
    std::string name;
    ScenarioResult result{ScenarioResult::Pass};
    std::string evidence;
    std::uint64_t recovery_cycles{0};
};

std::string to_string(DriveState value);
std::string to_string(ControlMode value);
std::string to_string(CommandType value);
std::string to_string(FaultCode value);
std::string to_string(FaultKind value);
std::string to_string(ScenarioResult value);

}  // namespace emrtc
