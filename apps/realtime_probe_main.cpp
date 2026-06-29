#include "emrtc/controller.hpp"
#include "emrtc/realtime.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

std::uint64_t parse_unsigned(const char* text, const std::string& option) {
    std::size_t consumed = 0;
    const std::string value(text);
    if (value.empty() || value.front() == '-') {
        throw std::invalid_argument(option + " requires a non-negative integer");
    }
    const auto parsed = std::stoull(value, &consumed);
    if (consumed != value.size()) throw std::invalid_argument(option + " requires an integer");
    return parsed;
}

emrtc::Nanoseconds microseconds_to_nanoseconds(std::uint64_t value,
                                              const std::string& option) {
    const auto maximum = static_cast<std::uint64_t>(
        std::numeric_limits<emrtc::Nanoseconds>::max());
    if (value > maximum / 1000U) {
        throw std::out_of_range(option + " is too large");
    }
    return static_cast<emrtc::Nanoseconds>(value * 1000U);
}

void print_help() {
    std::cout
        << "Usage: emrtc_realtime_probe [options]\n"
        << "  --cycles <count>\n"
        << "  --period-us <microseconds>\n"
        << "  --tolerance-us <microseconds>\n"
        << "  --stress-threads <count>\n"
        << "  --request-fifo [--fifo-priority <1..99>]\n"
        << "  --request-memory-lock\n"
        << "  --out <directory>\n";
}

}  // namespace

int main(int argc, char** argv) {
    emrtc::RealtimeProbeConfig config;
    std::string output_directory = "reports/generated/realtime-probe";
    try {
        for (int index = 1; index < argc; ++index) {
            const std::string option = argv[index];
            if (option == "--help") {
                print_help();
                return 0;
            }
            if (option == "--request-fifo") {
                config.request_fifo = true;
                continue;
            }
            if (option == "--request-memory-lock") {
                config.request_memory_lock = true;
                continue;
            }
            if (index + 1 >= argc) throw std::invalid_argument(option + " requires a value");
            const char* value = argv[++index];
            if (option == "--cycles") config.cycles = parse_unsigned(value, option);
            else if (option == "--period-us") config.period_ns = microseconds_to_nanoseconds(parse_unsigned(value, option), option);
            else if (option == "--tolerance-us") config.deadline_tolerance_ns = microseconds_to_nanoseconds(parse_unsigned(value, option), option);
            else if (option == "--stress-threads") {
                const auto parsed = parse_unsigned(value, option);
                if (parsed > std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range(option + " is too large");
                config.stress_threads = static_cast<std::uint32_t>(parsed);
            } else if (option == "--fifo-priority") {
                const auto parsed = parse_unsigned(value, option);
                if (parsed > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) throw std::out_of_range(option + " is too large");
                config.fifo_priority = static_cast<int>(parsed);
            }
            else if (option == "--out") output_directory = value;
            else throw std::invalid_argument("unknown option: " + option);
        }

        emrtc::RealTimeController controller;
        controller.command({emrtc::CommandType::Enable, 0.0});
        const auto metrics = emrtc::run_realtime_probe(
            config, [&controller](emrtc::CycleIndex) { controller.run_cycles(1); });
        if (!emrtc::write_realtime_probe_reports(metrics, output_directory)) {
            std::cerr << "failed to write real-time probe reports\n";
            return 1;
        }
        std::cout << "platform=" << metrics.capabilities.platform << '\n'
                  << "cycles=" << metrics.samples.size() << '\n'
                  << "min_period_ns=" << metrics.min_period_ns << '\n'
                  << "max_period_ns=" << metrics.max_period_ns << '\n'
                  << "mean_period_ns=" << static_cast<long long>(metrics.mean_period_ns) << '\n'
                  << "max_wake_lateness_ns=" << metrics.max_wake_lateness_ns << '\n'
                  << "deadline_overruns=" << metrics.deadline_overruns << '\n'
                  << "missed_cycles=" << metrics.missed_cycles << '\n'
                  << "fifo_enabled=" << (metrics.capabilities.fifo_enabled ? "true" : "false") << '\n'
                  << "memory_locked=" << (metrics.capabilities.memory_locked ? "true" : "false") << '\n'
                  << "reports=" << output_directory << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "realtime_probe_error=" << error.what() << '\n';
        return 1;
    }
}
