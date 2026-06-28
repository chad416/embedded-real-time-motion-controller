#include "emrtc/controller.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

emrtc::FaultKind parse_fault(const std::string& value) {
    if (value == "missed_cycle") return emrtc::FaultKind::MissedCycle;
    if (value == "cpu_load_spike") return emrtc::FaultKind::CpuLoadSpike;
    if (value == "heartbeat_timeout") return emrtc::FaultKind::HeartbeatTimeout;
    if (value == "bus_dropout") return emrtc::FaultKind::BusDropout;
    if (value == "packet_loss") return emrtc::FaultKind::PacketLoss;
    if (value == "invalid_drive_transition") return emrtc::FaultKind::InvalidDriveTransition;
    if (value == "encoder_feedback_loss") return emrtc::FaultKind::EncoderFeedbackLoss;
    if (value == "sensor_noise") return emrtc::FaultKind::SensorNoise;
    if (value == "following_error") return emrtc::FaultKind::FollowingError;
    if (value == "drive_fault") return emrtc::FaultKind::DriveFault;
    if (value == "watchdog_timeout") return emrtc::FaultKind::WatchdogTimeout;
    if (value == "command_abuse") return emrtc::FaultKind::CommandAbuse;
    return emrtc::FaultKind::DriveFault;
}

void print_help() {
    std::cout
        << "Usage:\n"
        << "  emrtc_cli show status\n"
        << "  emrtc_cli run scenario\n"
        << "  emrtc_cli inject fault <fault_name>\n"
        << "  emrtc_cli reset fault\n"
        << "  emrtc_cli export report [dir]\n";
}

}  // namespace

int main(int argc, char** argv) {
    emrtc::RealTimeController controller;
    controller.command({emrtc::CommandType::Enable, 0.0});
    controller.run_cycles(5);

    if (argc < 2) {
        print_help();
        return 0;
    }

    const std::string verb = argv[1];
    const std::string noun = argc > 2 ? argv[2] : "";

    if (verb == "show" && noun == "status") {
        const auto feedback = controller.axis().feedback();
        std::cout << "state=" << emrtc::to_string(feedback.state) << "\n";
        std::cout << "mode=" << emrtc::to_string(feedback.mode) << "\n";
        std::cout << "position=" << feedback.position << "\n";
        std::cout << "velocity=" << feedback.velocity << "\n";
        std::cout << "fault=" << emrtc::to_string(feedback.fault_code) << "\n";
        return 0;
    }

    if (verb == "run" && noun == "scenario") {
        const auto summaries = emrtc::run_validation_scenarios();
        for (const auto& summary : summaries) {
            std::cout << emrtc::to_string(summary.result) << " " << summary.name
                      << " - " << summary.evidence << "\n";
        }
        return 0;
    }

    if (verb == "inject" && noun == "fault" && argc > 3) {
        controller.fault_injector().schedule({6, parse_fault(argv[3]), 1.0, 8});
        controller.run_cycles(20);
        const auto feedback = controller.axis().feedback();
        std::cout << "fault=" << emrtc::to_string(feedback.fault_code) << "\n";
        return feedback.fault_code == emrtc::FaultCode::None ? 1 : 0;
    }

    if (verb == "reset" && noun == "fault") {
        controller.fault_injector().schedule({6, emrtc::FaultKind::DriveFault, 1.0, 1});
        controller.run_cycles(10);
        controller.reset_faults();
        controller.command({emrtc::CommandType::Enable, 0.0});
        controller.run_cycles(5);
        std::cout << "state=" << emrtc::to_string(controller.axis().state()) << "\n";
        return controller.axis().state() == emrtc::DriveState::OperationEnabled ? 0 : 1;
    }

    if (verb == "export" && noun == "report") {
        const std::string output_dir = argc > 3 ? argv[3] : "reports/generated";
        std::filesystem::create_directories(output_dir);
        emrtc::RuntimeMetrics metrics;
        emrtc::run_validation_scenarios(&metrics);
        emrtc::TelemetryRecorder::write_trace_csv(metrics, output_dir + "/trace.csv");
        emrtc::TelemetryRecorder::write_metrics_json(metrics, output_dir + "/metrics.json");
        emrtc::TelemetryRecorder::write_fault_log_json(metrics, output_dir + "/fault_log.json");
        emrtc::TelemetryRecorder::write_validation_markdown(metrics, output_dir + "/validation_report.md");
        std::cout << "report exported to " << output_dir << "\n";
        return 0;
    }

    print_help();
    return 1;
}
