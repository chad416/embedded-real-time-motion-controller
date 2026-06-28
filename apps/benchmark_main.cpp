#include "emrtc/controller.hpp"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::uint64_t cycles = 10000;
    std::string output_dir = "reports/generated";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--cycles" && i + 1 < argc) {
            cycles = static_cast<std::uint64_t>(std::stoull(argv[++i]));
        } else if (arg == "--out" && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: emrtc_benchmark [--cycles N] [--out DIR]\n";
            return 0;
        }
    }

    std::filesystem::create_directories(output_dir);
    const auto metrics = emrtc::run_benchmark(cycles);
    emrtc::TelemetryRecorder::write_metrics_json(metrics, output_dir + "/benchmark_metrics.json");
    emrtc::TelemetryRecorder::write_trace_csv(metrics, output_dir + "/benchmark_trace.csv");

    std::cout << "Benchmark complete\n";
    std::cout << "cycles=" << metrics.cycles.size() << "\n";
    std::cout << "min_cycle_ns=" << metrics.min_cycle_ns << "\n";
    std::cout << "max_cycle_ns=" << metrics.max_cycle_ns << "\n";
    std::cout << "mean_cycle_ns=" << static_cast<long long>(metrics.mean_cycle_ns) << "\n";
    std::cout << "missed_cycles=" << metrics.missed_cycles << "\n";
    std::cout << "deadline_overruns=" << metrics.deadline_overruns << "\n";
    return metrics.deadline_overruns <= 2 ? 0 : 1;
}
