#include "emrtc/controller.hpp"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::uint64_t cycles = 2500;
    std::string output_dir = "reports/generated";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--cycles" && i + 1 < argc) {
            cycles = static_cast<std::uint64_t>(std::stoull(argv[++i]));
        } else if (arg == "--out" && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: emrtc_sim [--cycles N] [--out DIR]\n";
            return 0;
        }
    }

    std::filesystem::create_directories(output_dir);
    emrtc::RuntimeMetrics metrics;
    const auto scenarios = emrtc::run_validation_scenarios(&metrics);
    (void)scenarios;

    if (cycles != metrics.cycles.size()) {
        metrics = emrtc::run_benchmark(cycles);
        metrics.scenarios = emrtc::run_validation_scenarios();
    }

    emrtc::TelemetryRecorder::write_trace_csv(metrics, output_dir + "/trace.csv");
    emrtc::TelemetryRecorder::write_metrics_json(metrics, output_dir + "/metrics.json");
    emrtc::TelemetryRecorder::write_fault_log_json(metrics, output_dir + "/fault_log.json");
    emrtc::TelemetryRecorder::write_validation_markdown(metrics, output_dir + "/validation_report.md");

    std::cout << "Simulation complete\n";
    std::cout << "cycles=" << metrics.cycles.size()
              << " missed=" << metrics.missed_cycles
              << " deadline_overruns=" << metrics.deadline_overruns
              << " packet_loss=" << metrics.bus.packet_loss_count << "\n";
    std::cout << "reports=" << output_dir << "\n";
    return 0;
}
