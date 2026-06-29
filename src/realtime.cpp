#include "emrtc/realtime.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <thread>

#ifdef __linux__
#include <cerrno>
#include <cstring>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <time.h>
#endif

namespace emrtc {
namespace {

Nanoseconds monotonic_now_ns() {
#ifdef __linux__
    struct timespec value {};
    if (::clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        throw std::runtime_error("clock_gettime(CLOCK_MONOTONIC) failed");
    }
    return static_cast<Nanoseconds>(value.tv_sec) * 1'000'000'000LL + value.tv_nsec;
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
#endif
}

void sleep_until_ns(Nanoseconds deadline_ns) {
#ifdef __linux__
    struct timespec deadline {};
    deadline.tv_sec = static_cast<time_t>(deadline_ns / 1'000'000'000LL);
    deadline.tv_nsec = static_cast<long>(deadline_ns % 1'000'000'000LL);
    int result = 0;
    do {
        result = ::clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, nullptr);
    } while (result == EINTR);
    if (result != 0) {
        throw std::runtime_error("clock_nanosleep failed: " + std::string(std::strerror(result)));
    }
#else
    const auto deadline = std::chrono::steady_clock::time_point(
        std::chrono::nanoseconds(deadline_ns));
    std::this_thread::sleep_until(deadline);
#endif
}

RealtimeCapabilityStatus prepare_realtime(const RealtimeProbeConfig& config) {
    RealtimeCapabilityStatus status;
#ifdef __linux__
    status.platform = "linux";
#elif defined(_WIN32)
    status.platform = "windows";
#else
    status.platform = "portable_host";
#endif
    status.monotonic_clock = true;
    status.absolute_sleep = true;
    status.fifo_requested = config.request_fifo;
    status.memory_lock_requested = config.request_memory_lock;
    std::ostringstream detail;

#ifdef __linux__
    if (config.request_memory_lock) {
        if (::mlockall(MCL_CURRENT | MCL_FUTURE) == 0) {
            status.memory_locked = true;
            detail << "mlockall=enabled; ";
        } else {
            detail << "mlockall=unavailable(" << std::strerror(errno) << "); ";
        }
    }
    if (config.request_fifo) {
        const int minimum = ::sched_get_priority_min(SCHED_FIFO);
        const int maximum = ::sched_get_priority_max(SCHED_FIFO);
        if (config.fifo_priority < minimum || config.fifo_priority > maximum) {
            detail << "SCHED_FIFO=invalid_priority; ";
        } else {
            struct sched_param parameter {};
            parameter.sched_priority = config.fifo_priority;
            const int result = ::pthread_setschedparam(pthread_self(), SCHED_FIFO, &parameter);
            if (result == 0) {
                status.fifo_enabled = true;
                detail << "SCHED_FIFO=enabled; ";
            } else {
                detail << "SCHED_FIFO=unavailable(" << std::strerror(result) << "); ";
            }
        }
    }
#else
    if (config.request_memory_lock) detail << "memory_lock=unsupported_on_this_host; ";
    if (config.request_fifo) detail << "SCHED_FIFO=unsupported_on_this_host; ";
#endif
    if (detail.str().empty()) detail << "portable monotonic absolute-deadline mode";
    status.detail = detail.str();
    return status;
}

void restore_realtime(const RealtimeCapabilityStatus& status) {
#ifdef __linux__
    if (status.fifo_enabled) {
        struct sched_param parameter {};
        parameter.sched_priority = 0;
        (void)::pthread_setschedparam(pthread_self(), SCHED_OTHER, &parameter);
    }
    if (status.memory_locked) (void)::munlockall();
#else
    (void)status;
#endif
}

void finalize_metrics(RealtimeProbeMetrics& metrics) {
    if (metrics.samples.empty()) return;
    metrics.min_period_ns = metrics.samples.front().measured_period_ns;
    metrics.max_period_ns = metrics.samples.front().measured_period_ns;
    metrics.min_jitter_ns = metrics.samples.front().jitter_ns;
    metrics.max_jitter_ns = metrics.samples.front().jitter_ns;
    long double total_period = 0.0;
    long double total_abs_jitter = 0.0;
    for (const auto& sample : metrics.samples) {
        metrics.min_period_ns = std::min(metrics.min_period_ns, sample.measured_period_ns);
        metrics.max_period_ns = std::max(metrics.max_period_ns, sample.measured_period_ns);
        metrics.min_jitter_ns = std::min(metrics.min_jitter_ns, sample.jitter_ns);
        metrics.max_jitter_ns = std::max(metrics.max_jitter_ns, sample.jitter_ns);
        metrics.max_wake_lateness_ns =
            std::max(metrics.max_wake_lateness_ns, sample.wake_lateness_ns);
        total_period += static_cast<long double>(sample.measured_period_ns);
        total_abs_jitter += std::abs(static_cast<long double>(sample.jitter_ns));
        if (sample.deadline_overrun) ++metrics.deadline_overruns;
        if (sample.missed_cycle) ++metrics.missed_cycles;
    }
    metrics.mean_period_ns =
        static_cast<double>(total_period / metrics.samples.size());
    metrics.mean_abs_jitter_ns =
        static_cast<double>(total_abs_jitter / metrics.samples.size());
}

}  // namespace

RealtimeProbeMetrics run_realtime_probe(const RealtimeProbeConfig& config,
                                        const RealtimeCycleCallback& cycle_callback) {
    if (config.cycles == 0U || config.period_ns <= 0 ||
        config.deadline_tolerance_ns < 0 || config.stress_threads > 64U) {
        throw std::invalid_argument("invalid real-time probe configuration");
    }

    std::atomic<bool> stop_stress{false};
    std::vector<std::thread> stress_workers;
    stress_workers.reserve(config.stress_threads);
    for (std::uint32_t index = 0; index < config.stress_threads; ++index) {
        stress_workers.emplace_back([&stop_stress, index]() {
            volatile std::uint64_t value = index + 1U;
            while (!stop_stress.load(std::memory_order_relaxed)) {
                value = value * 6364136223846793005ULL + 1442695040888963407ULL;
                std::atomic_signal_fence(std::memory_order_seq_cst);
            }
            (void)value;
        });
    }

    RealtimeProbeMetrics metrics;
    metrics.config = config;
    metrics.samples.reserve(static_cast<std::size_t>(config.cycles));
    metrics.capabilities = prepare_realtime(config);
    try {
        Nanoseconds previous_wake = monotonic_now_ns();
        Nanoseconds deadline = previous_wake + config.period_ns;
        for (CycleIndex cycle = 0; cycle < config.cycles; ++cycle) {
            sleep_until_ns(deadline);
            const Nanoseconds wake = monotonic_now_ns();
            const Nanoseconds measured = wake - previous_wake;
            const Nanoseconds jitter = measured - config.period_ns;
            const Nanoseconds lateness = std::max<Nanoseconds>(0, wake - deadline);
            metrics.samples.push_back({
                cycle,
                wake,
                measured,
                jitter,
                lateness,
                measured > config.period_ns + config.deadline_tolerance_ns,
                measured >= config.period_ns * 2});
            if (cycle_callback) cycle_callback(cycle);
            previous_wake = wake;
            deadline += config.period_ns;
        }
    } catch (...) {
        restore_realtime(metrics.capabilities);
        stop_stress.store(true, std::memory_order_relaxed);
        for (auto& worker : stress_workers) worker.join();
        throw;
    }
    restore_realtime(metrics.capabilities);
    stop_stress.store(true, std::memory_order_relaxed);
    for (auto& worker : stress_workers) worker.join();
    finalize_metrics(metrics);
    return metrics;
}

bool write_realtime_probe_reports(const RealtimeProbeMetrics& metrics,
                                  const std::string& output_directory) {
    std::filesystem::create_directories(output_directory);
    std::ofstream csv(output_directory + "/realtime_probe.csv");
    std::ofstream json(output_directory + "/realtime_probe.json");
    std::ofstream markdown(output_directory + "/realtime_probe.md");
    if (!csv || !json || !markdown) return false;

    csv << "cycle,wake_timestamp_ns,measured_period_ns,jitter_ns,wake_lateness_ns,deadline_overrun,missed_cycle\n";
    for (const auto& sample : metrics.samples) {
        csv << sample.cycle << ',' << sample.wake_timestamp_ns << ','
            << sample.measured_period_ns << ',' << sample.jitter_ns << ','
            << sample.wake_lateness_ns << ',' << (sample.deadline_overrun ? 1 : 0)
            << ',' << (sample.missed_cycle ? 1 : 0) << '\n';
    }

    json << "{\n"
         << "  \"platform\": \"" << metrics.capabilities.platform << "\",\n"
         << "  \"cycle_count\": " << metrics.samples.size() << ",\n"
         << "  \"period_ns\": " << metrics.config.period_ns << ",\n"
         << "  \"min_period_ns\": " << metrics.min_period_ns << ",\n"
         << "  \"max_period_ns\": " << metrics.max_period_ns << ",\n"
         << "  \"mean_period_ns\": " << std::setprecision(12) << metrics.mean_period_ns << ",\n"
         << "  \"min_jitter_ns\": " << metrics.min_jitter_ns << ",\n"
         << "  \"max_jitter_ns\": " << metrics.max_jitter_ns << ",\n"
         << "  \"mean_abs_jitter_ns\": " << metrics.mean_abs_jitter_ns << ",\n"
         << "  \"max_wake_lateness_ns\": " << metrics.max_wake_lateness_ns << ",\n"
         << "  \"deadline_overruns\": " << metrics.deadline_overruns << ",\n"
         << "  \"missed_cycles\": " << metrics.missed_cycles << ",\n"
         << "  \"fifo_requested\": " << (metrics.capabilities.fifo_requested ? "true" : "false") << ",\n"
         << "  \"fifo_enabled\": " << (metrics.capabilities.fifo_enabled ? "true" : "false") << ",\n"
         << "  \"memory_lock_requested\": " << (metrics.capabilities.memory_lock_requested ? "true" : "false") << ",\n"
         << "  \"memory_locked\": " << (metrics.capabilities.memory_locked ? "true" : "false") << "\n"
         << "}\n";

    markdown << "# Host Real-Time Probe\n\n"
             << "This report contains wall-clock host measurements, not synthetic simulation timing and not hardware commissioning evidence.\n\n"
             << "| Metric | Value |\n| --- | ---: |\n"
             << "| Platform | " << metrics.capabilities.platform << " |\n"
             << "| Cycles | " << metrics.samples.size() << " |\n"
             << "| Requested period (ns) | " << metrics.config.period_ns << " |\n"
             << "| Minimum period (ns) | " << metrics.min_period_ns << " |\n"
             << "| Maximum period (ns) | " << metrics.max_period_ns << " |\n"
             << "| Mean period (ns) | " << static_cast<long long>(metrics.mean_period_ns) << " |\n"
             << "| Mean absolute jitter (ns) | " << static_cast<long long>(metrics.mean_abs_jitter_ns) << " |\n"
             << "| Maximum wake lateness (ns) | " << metrics.max_wake_lateness_ns << " |\n"
             << "| Deadline overruns | " << metrics.deadline_overruns << " |\n"
             << "| Missed cycles | " << metrics.missed_cycles << " |\n"
             << "| SCHED_FIFO enabled | " << (metrics.capabilities.fifo_enabled ? "true" : "false") << " |\n"
             << "| Memory locked | " << (metrics.capabilities.memory_locked ? "true" : "false") << " |\n\n"
             << "Capability detail: " << metrics.capabilities.detail << "\n";
    return true;
}

}  // namespace emrtc
