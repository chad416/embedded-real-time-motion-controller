#include "emrtc/replay_adapter.hpp"

#include "emrtc/process_image_mapping.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace emrtc {
namespace {

constexpr std::size_t kReplayColumnCount = 13;

std::string trim(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

std::vector<std::string> split_csv_row(const std::string& row) {
    std::vector<std::string> fields;
    std::stringstream stream(row);
    std::string field;
    while (std::getline(stream, field, ',')) {
        fields.push_back(trim(field));
    }
    if (!row.empty() && row.back() == ',') {
        fields.emplace_back();
    }
    return fields;
}

bool parse_bool(const std::string& value, std::size_t line) {
    if (value == "1" || value == "true") return true;
    if (value == "0" || value == "false") return false;
    throw std::runtime_error("invalid boolean at replay line " + std::to_string(line));
}

std::uint64_t parse_u64(const std::string& value, std::size_t line) {
    if (value.empty() || value.front() == '-') {
        throw std::runtime_error("invalid unsigned integer at replay line " +
                                 std::to_string(line));
    }
    try {
        std::size_t parsed = 0;
        const auto result = std::stoull(value, &parsed, 0);
        if (parsed != value.size()) {
            throw std::runtime_error("trailing characters");
        }
        return result;
    } catch (const std::exception&) {
        throw std::runtime_error("invalid unsigned integer at replay line " +
                                 std::to_string(line));
    }
}

std::int32_t parse_i32(const std::string& value, std::size_t line) {
    try {
        std::size_t parsed = 0;
        const auto result = std::stoll(value, &parsed, 0);
        if (parsed != value.size() ||
            result < std::numeric_limits<std::int32_t>::min() ||
            result > std::numeric_limits<std::int32_t>::max()) {
            throw std::runtime_error("out of range");
        }
        return static_cast<std::int32_t>(result);
    } catch (const std::exception&) {
        throw std::runtime_error("invalid signed 32-bit integer at replay line " +
                                 std::to_string(line));
    }
}

std::uint16_t parse_u16(const std::string& value, std::size_t line) {
    const auto result = parse_u64(value, line);
    if (result > std::numeric_limits<std::uint16_t>::max()) {
        throw std::runtime_error("invalid unsigned 16-bit integer at replay line " +
                                 std::to_string(line));
    }
    return static_cast<std::uint16_t>(result);
}

FaultCode parse_fault_code(const std::string& value, std::size_t line) {
    if (value == "none") return FaultCode::None;
    if (value == "missed_cycle") return FaultCode::MissedCycle;
    if (value == "deadline_overrun") return FaultCode::DeadlineOverrun;
    if (value == "heartbeat_timeout") return FaultCode::HeartbeatTimeout;
    if (value == "bus_dropout") return FaultCode::BusDropout;
    if (value == "packet_loss") return FaultCode::PacketLoss;
    if (value == "invalid_drive_transition") return FaultCode::InvalidDriveTransition;
    if (value == "encoder_feedback_loss") return FaultCode::EncoderFeedbackLoss;
    if (value == "following_error") return FaultCode::FollowingError;
    if (value == "drive_fault") return FaultCode::DriveFault;
    if (value == "watchdog_timeout") return FaultCode::WatchdogTimeout;
    if (value == "command_rejected") return FaultCode::CommandRejected;
    if (value == "emergency_stop") return FaultCode::EmergencyStop;
    throw std::runtime_error("invalid fault code at replay line " + std::to_string(line));
}

FaultCode observed_fault(const AdapterContractResult& result) {
    return result.ok ? FaultCode::None : result.fault;
}

}  // namespace

ReplayBusBackend::ReplayBusBackend(std::vector<ReplayFrame> frames, double counts_per_unit)
    : frames_(std::move(frames)),
      counts_per_unit_(counts_per_unit > 0.0 ? counts_per_unit : 1.0) {}

BusExchangeResult ReplayBusBackend::invalid_exchange() {
    diagnostics_.invalid_response_count++;
    return {false, diagnostics_.heartbeat_ok, true};
}

BusExchangeResult ReplayBusBackend::exchange(CycleIndex cycle,
                                              std::vector<ProcessImage>& images) {
    diagnostics_.tx_packets++;
    if (next_frame_ >= frames_.size() || images.size() != 1U) {
        return invalid_exchange();
    }

    const auto& frame = frames_[next_frame_++];
    if (frame.cycle != cycle) {
        return invalid_exchange();
    }

    const auto outgoing = encode_cia402_wire_image(images.front(), counts_per_unit_);
    if (outgoing.controlword != frame.expected_controlword ||
        outgoing.target_position_counts != frame.expected_target_position_counts ||
        outgoing.target_velocity_counts != frame.expected_target_velocity_counts) {
        return invalid_exchange();
    }

    diagnostics_.online = frame.online;
    diagnostics_.heartbeat_ok = frame.heartbeat_ok;
    if (!frame.online) {
        diagnostics_.dropout_count++;
        if (previous_online_) {
            diagnostics_.cable_disconnect_events++;
        }
        previous_online_ = false;
        return {false, frame.heartbeat_ok, false};
    }
    previous_online_ = true;

    if (!frame.heartbeat_ok) {
        diagnostics_.heartbeat_timeout_count++;
    } else {
        diagnostics_.last_heartbeat_cycle = cycle;
    }

    if (frame.invalid_response) {
        return invalid_exchange();
    }

    if (frame.rx_update) {
        Cia402WireImage incoming;
        incoming.controlword = outgoing.controlword;
        incoming.statusword = frame.statusword;
        incoming.target_position_counts = outgoing.target_position_counts;
        incoming.target_velocity_counts = outgoing.target_velocity_counts;
        incoming.actual_position_counts = frame.actual_position_counts;
        incoming.actual_velocity_counts = frame.actual_velocity_counts;
        incoming.drive_fault = frame.drive_fault;
        images.front() = decode_cia402_wire_image(incoming, counts_per_unit_);
        diagnostics_.rx_packets++;
    }

    return {frame.heartbeat_ok, frame.heartbeat_ok, false};
}

const BusDiagnostics& ReplayBusBackend::diagnostics() const {
    return diagnostics_;
}

const std::vector<ReplayFrame>& ReplayBusBackend::frames() const {
    return frames_;
}

std::size_t ReplayBusBackend::frames_consumed() const {
    return next_frame_;
}

bool ReplayBusBackend::complete() const {
    return next_frame_ == frames_.size();
}

std::vector<ReplayFrame> load_replay_frames_csv(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open replay file: " + path);
    }

    std::string line;
    if (!std::getline(input, line)) {
        throw std::runtime_error("replay file is empty: " + path);
    }
    const auto header = split_csv_row(line);
    const std::vector<std::string> expected_header{
        "cycle",
        "rx_update",
        "online",
        "heartbeat_ok",
        "invalid_response",
        "statusword",
        "actual_position_counts",
        "actual_velocity_counts",
        "drive_fault",
        "expected_controlword",
        "expected_target_position_counts",
        "expected_target_velocity_counts",
        "expected_contract_fault",
    };
    if (header != expected_header) {
        throw std::runtime_error("replay CSV header does not match the adapter contract");
    }

    std::vector<ReplayFrame> frames;
    std::size_t line_number = 1;
    while (std::getline(input, line)) {
        line_number++;
        if (trim(line).empty()) {
            continue;
        }
        const auto fields = split_csv_row(line);
        if (fields.size() != kReplayColumnCount) {
            throw std::runtime_error("wrong replay column count at line " +
                                     std::to_string(line_number));
        }

        ReplayFrame frame;
        frame.cycle = parse_u64(fields[0], line_number);
        frame.rx_update = parse_bool(fields[1], line_number);
        frame.online = parse_bool(fields[2], line_number);
        frame.heartbeat_ok = parse_bool(fields[3], line_number);
        frame.invalid_response = parse_bool(fields[4], line_number);
        frame.statusword = parse_u16(fields[5], line_number);
        frame.actual_position_counts = parse_i32(fields[6], line_number);
        frame.actual_velocity_counts = parse_i32(fields[7], line_number);
        frame.drive_fault = parse_fault_code(fields[8], line_number);
        frame.expected_controlword = parse_u16(fields[9], line_number);
        frame.expected_target_position_counts = parse_i32(fields[10], line_number);
        frame.expected_target_velocity_counts = parse_i32(fields[11], line_number);
        frame.expected_contract_fault = parse_fault_code(fields[12], line_number);
        if (!frames.empty() && frame.cycle <= frames.back().cycle) {
            throw std::runtime_error("replay cycles must be strictly increasing");
        }
        frames.push_back(frame);
    }

    if (frames.empty()) {
        throw std::runtime_error("replay file contains no frames: " + path);
    }
    return frames;
}

ReplayRunSummary run_replay_file(const std::string& path,
                                 double counts_per_unit,
                                 std::uint32_t stale_rx_limit_cycles) {
    auto frames = load_replay_frames_csv(path);
    ReplayBusBackend backend(std::move(frames), counts_per_unit);
    AdapterExchangeMonitor monitor(stale_rx_limit_cycles);
    std::vector<ProcessImage> images(1);
    ReplayRunSummary summary;

    for (const auto& frame : backend.frames()) {
        images.front().controlword = frame.expected_controlword;
        images.front().target_position =
            counts_to_units(frame.expected_target_position_counts, counts_per_unit);
        images.front().target_velocity =
            counts_to_units(frame.expected_target_velocity_counts, counts_per_unit);

        const auto exchange = backend.exchange(frame.cycle, images);
        const auto contract = monitor.observe(frame.cycle, exchange, backend.diagnostics());
        const auto fault = observed_fault(contract);
        if (fault != FaultCode::None) {
            summary.contract_faults_observed++;
        }
        if (fault != frame.expected_contract_fault) {
            summary.expectation_mismatches++;
            if (summary.first_mismatch.empty()) {
                std::ostringstream detail;
                detail << "cycle " << frame.cycle
                       << " expected " << to_string(frame.expected_contract_fault)
                       << " observed " << to_string(fault)
                       << " (" << contract.detail << ")";
                summary.first_mismatch = detail.str();
            }
        }
        summary.frames_processed++;
    }

    summary.final_image = images.front();
    summary.diagnostics = backend.diagnostics();
    summary.pass = backend.complete() && summary.expectation_mismatches == 0;
    return summary;
}

}  // namespace emrtc
