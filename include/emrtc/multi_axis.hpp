#pragma once

#include "emrtc/axis.hpp"
#include "emrtc/types.hpp"

#include <cstdint>

namespace emrtc {

struct CoordinatedMoveConfig {
    double x_target{1.0};
    double y_target{1.0};
    Nanoseconds cycle_time_ns{1'000'000};
    std::uint64_t cycles{1200};
    double path_error_limit{0.010};
    double target_tolerance{0.005};
    double velocity_settle_tolerance{0.010};
    double settling_time_limit_s{1.200};
};

struct CoordinatedMoveMetrics {
    std::uint64_t cycle_count{0};
    double target_x{0.0};
    double target_y{0.0};
    double final_x{0.0};
    double final_y{0.0};
    double final_x_error{0.0};
    double final_y_error{0.0};
    double final_position_error{0.0};
    double rms_path_error{0.0};
    double max_path_error{0.0};
    double path_error_limit{0.0};
    double settling_time_s{0.0};
    double settling_time_limit_s{0.0};
    double target_tolerance{0.0};
    bool target_reached{false};
    bool path_within_limit{false};
    bool acceptance_pass{false};
};

CoordinatedMoveMetrics simulate_two_axis_linear_move(const CoordinatedMoveConfig& config = {});

}  // namespace emrtc
