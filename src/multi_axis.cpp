#include "emrtc/multi_axis.hpp"

#include <algorithm>
#include <cmath>

namespace emrtc {
namespace {

void enable_axis(Axis& axis, Nanoseconds cycle_time_ns, CycleIndex& cycle) {
    axis.apply_command({CommandType::Enable, 0.0}, cycle);
    for (int i = 0; i < 5; ++i) {
        axis.update(static_cast<double>(cycle_time_ns) * 1.0e-9, cycle++);
    }
}

double path_error(double x, double y, double x_target, double y_target) {
    const double norm = std::sqrt(x_target * x_target + y_target * y_target);
    if (norm <= 1.0e-12) {
        return std::sqrt(x * x + y * y);
    }
    return std::abs(y * x_target - x * y_target) / norm;
}

}  // namespace

CoordinatedMoveMetrics simulate_two_axis_linear_move(const CoordinatedMoveConfig& config) {
    Axis x_axis("x_axis");
    Axis y_axis("y_axis");
    CycleIndex cycle = 0;
    enable_axis(x_axis, config.cycle_time_ns, cycle);
    enable_axis(y_axis, config.cycle_time_ns, cycle);

    x_axis.apply_command({CommandType::MoveAbsolute, config.x_target}, cycle);
    y_axis.apply_command({CommandType::MoveAbsolute, config.y_target}, cycle);

    CoordinatedMoveMetrics metrics;
    metrics.target_x = config.x_target;
    metrics.target_y = config.y_target;
    metrics.path_error_limit = config.path_error_limit;
    metrics.settling_time_limit_s = config.settling_time_limit_s;
    metrics.target_tolerance = config.target_tolerance;
    double squared_error_sum = 0.0;
    bool settled = false;

    for (std::uint64_t i = 0; i < config.cycles; ++i) {
        x_axis.update(static_cast<double>(config.cycle_time_ns) * 1.0e-9, cycle);
        y_axis.update(static_cast<double>(config.cycle_time_ns) * 1.0e-9, cycle);
        const auto x = x_axis.feedback().position;
        const auto y = y_axis.feedback().position;
        const auto error = path_error(x, y, config.x_target, config.y_target);
        squared_error_sum += error * error;
        metrics.max_path_error = std::max(metrics.max_path_error, error);

        if (!settled &&
            std::abs(config.x_target - x) <= config.target_tolerance &&
            std::abs(config.y_target - y) <= config.target_tolerance &&
            std::abs(x_axis.feedback().velocity) <= config.velocity_settle_tolerance &&
            std::abs(y_axis.feedback().velocity) <= config.velocity_settle_tolerance) {
            metrics.settling_time_s = static_cast<double>(i + 1) *
                                      static_cast<double>(config.cycle_time_ns) * 1.0e-9;
            settled = true;
        }
        cycle++;
    }

    metrics.cycle_count = config.cycles;
    metrics.final_x = x_axis.feedback().position;
    metrics.final_y = y_axis.feedback().position;
    metrics.final_x_error = config.x_target - metrics.final_x;
    metrics.final_y_error = config.y_target - metrics.final_y;
    metrics.final_position_error = std::sqrt(metrics.final_x_error * metrics.final_x_error +
                                             metrics.final_y_error * metrics.final_y_error);
    metrics.rms_path_error = std::sqrt(squared_error_sum / static_cast<double>(std::max<std::uint64_t>(1, config.cycles)));
    metrics.target_reached = settled;
    metrics.path_within_limit = metrics.max_path_error <= config.path_error_limit;
    metrics.acceptance_pass = metrics.target_reached &&
                              metrics.path_within_limit &&
                              metrics.final_position_error <= config.target_tolerance &&
                              metrics.settling_time_s <= config.settling_time_limit_s;
    return metrics;
}

}  // namespace emrtc
