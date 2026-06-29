#pragma once

#include "emrtc/types.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace emrtc {

struct RealtimeProbeConfig {
    std::uint64_t cycles{1000};
    Nanoseconds period_ns{1'000'000};
    Nanoseconds deadline_tolerance_ns{100'000};
    std::uint32_t stress_threads{0};
    bool request_fifo{false};
    int fifo_priority{80};
    bool request_memory_lock{false};
};

struct RealtimeCapabilityStatus {
    std::string platform;
    bool monotonic_clock{false};
    bool absolute_sleep{false};
    bool fifo_requested{false};
    bool fifo_enabled{false};
    bool memory_lock_requested{false};
    bool memory_locked{false};
    std::string detail;
};

struct RealtimeProbeSample {
    CycleIndex cycle{0};
    Nanoseconds wake_timestamp_ns{0};
    Nanoseconds measured_period_ns{0};
    Nanoseconds jitter_ns{0};
    Nanoseconds wake_lateness_ns{0};
    bool deadline_overrun{false};
    bool missed_cycle{false};
};

struct RealtimeProbeMetrics {
    RealtimeProbeConfig config;
    RealtimeCapabilityStatus capabilities;
    std::vector<RealtimeProbeSample> samples;
    Nanoseconds min_period_ns{0};
    Nanoseconds max_period_ns{0};
    double mean_period_ns{0.0};
    Nanoseconds min_jitter_ns{0};
    Nanoseconds max_jitter_ns{0};
    double mean_abs_jitter_ns{0.0};
    Nanoseconds max_wake_lateness_ns{0};
    std::uint64_t deadline_overruns{0};
    std::uint64_t missed_cycles{0};
};

using RealtimeCycleCallback = std::function<void(CycleIndex)>;

RealtimeProbeMetrics run_realtime_probe(
    const RealtimeProbeConfig& config,
    const RealtimeCycleCallback& cycle_callback = {});
bool write_realtime_probe_reports(const RealtimeProbeMetrics& metrics,
                                  const std::string& output_directory);

}  // namespace emrtc
