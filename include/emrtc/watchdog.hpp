#pragma once

#include "emrtc/bus.hpp"
#include "emrtc/types.hpp"

namespace emrtc {

struct WatchdogStatus {
    bool loop_ok{true};
    bool bus_ok{true};
    bool heartbeat_ok{true};
    bool following_error_ok{true};
    bool encoder_ok{true};
    FaultCode fault{FaultCode::None};
};

class WatchdogSupervisor {
public:
    explicit WatchdogSupervisor(ControllerConfig config);

    WatchdogStatus evaluate(const CycleSample& sample,
                            const BusDiagnostics& bus,
                            const AxisFeedback& feedback);

    void note_encoder_feedback_loss(bool lost, double dt_s);
    void reset();

private:
    ControllerConfig config_;
    std::uint32_t consecutive_missed_cycles_{0};
    std::uint32_t heartbeat_missing_cycles_{0};
    double encoder_loss_s_{0.0};
};

}  // namespace emrtc
