#include "emrtc/watchdog.hpp"

#include <cmath>

namespace emrtc {

WatchdogSupervisor::WatchdogSupervisor(ControllerConfig config)
    : config_(config) {}

WatchdogStatus WatchdogSupervisor::evaluate(const CycleSample& sample,
                                             const BusDiagnostics& bus,
                                             const AxisFeedback& feedback) {
    WatchdogStatus status;

    if (sample.missed_cycle || sample.deadline_overrun) {
        consecutive_missed_cycles_++;
    } else {
        consecutive_missed_cycles_ = 0;
    }

    if (!bus.heartbeat_ok) {
        heartbeat_missing_cycles_++;
    } else {
        heartbeat_missing_cycles_ = 0;
    }

    status.loop_ok = consecutive_missed_cycles_ < config_.loop_watchdog_limit_cycles;
    status.bus_ok = bus.online;
    status.heartbeat_ok = heartbeat_missing_cycles_ < config_.heartbeat_timeout_cycles;
    status.following_error_ok = std::abs(feedback.following_error) <= config_.following_error_limit;
    status.encoder_ok = encoder_loss_s_ < config_.encoder_loss_timeout_s;

    if (!status.loop_ok) {
        status.fault = FaultCode::WatchdogTimeout;
    } else if (!status.bus_ok) {
        status.fault = FaultCode::BusDropout;
    } else if (!status.heartbeat_ok) {
        status.fault = FaultCode::HeartbeatTimeout;
    } else if (!status.encoder_ok) {
        status.fault = FaultCode::EncoderFeedbackLoss;
    } else if (!status.following_error_ok) {
        status.fault = FaultCode::FollowingError;
    }

    return status;
}

void WatchdogSupervisor::note_encoder_feedback_loss(bool lost, double dt_s) {
    if (lost) {
        encoder_loss_s_ += dt_s;
    } else {
        encoder_loss_s_ = 0.0;
    }
}

void WatchdogSupervisor::reset() {
    consecutive_missed_cycles_ = 0;
    heartbeat_missing_cycles_ = 0;
    encoder_loss_s_ = 0.0;
}

}  // namespace emrtc
