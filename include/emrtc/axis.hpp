#pragma once

#include "emrtc/types.hpp"

#include <string>
#include <vector>

namespace emrtc {

struct AxisLimits {
    double max_velocity_units_per_s{2.0};
    double max_accel_units_per_s2{40.0};
    double following_error_limit{0.050};
};

class Axis {
public:
    explicit Axis(std::string name, AxisLimits limits = {});

    const std::string& name() const;
    DriveState state() const;
    ControlMode mode() const;
    FaultCode fault_code() const;
    AxisFeedback feedback() const;

    bool apply_command(const AxisCommand& command, CycleIndex cycle);
    void update(double dt_s, CycleIndex cycle);
    void set_bus_online(bool online);
    void request_safe_stop(CycleIndex cycle);
    void inject_fault(FaultKind kind, double magnitude, CycleIndex cycle);
    void force_fault(FaultCode code, CycleIndex cycle, const std::string& reason);
    void clear_transient_faults();
    void cold_restart();
    void warm_restart();

    const std::vector<std::string>& transition_trace() const;

private:
    void set_state(DriveState next, CycleIndex cycle, const std::string& reason);
    void enter_fault(FaultCode code, CycleIndex cycle, const std::string& reason);
    std::uint16_t build_statusword() const;
    std::uint16_t build_controlword() const;
    void update_motion(double dt_s);

    std::string name_;
    AxisLimits limits_;
    DriveState state_{DriveState::NotReady};
    ControlMode mode_{ControlMode::Disabled};
    FaultCode fault_code_{FaultCode::None};
    bool enable_requested_{false};
    bool bus_online_{true};
    bool encoder_feedback_lost_{false};
    double sensor_noise_{0.0};
    double injected_following_error_{0.0};
    double position_{0.0};
    double encoder_position_{0.0};
    double velocity_{0.0};
    double target_position_{0.0};
    double target_velocity_{0.0};
    bool target_reached_{true};
    std::vector<std::string> transition_trace_;
};

}  // namespace emrtc
