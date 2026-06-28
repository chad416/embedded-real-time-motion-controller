#pragma once

#include "emrtc/adapter_contract.hpp"
#include "emrtc/bus.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace emrtc {

struct ReplayFrame {
    CycleIndex cycle{0};
    bool rx_update{true};
    bool online{true};
    bool heartbeat_ok{true};
    bool invalid_response{false};
    std::uint16_t statusword{0};
    std::int32_t actual_position_counts{0};
    std::int32_t actual_velocity_counts{0};
    FaultCode drive_fault{FaultCode::None};
    std::uint16_t expected_controlword{0};
    std::int32_t expected_target_position_counts{0};
    std::int32_t expected_target_velocity_counts{0};
    FaultCode expected_contract_fault{FaultCode::None};
};

struct ReplayRunSummary {
    bool pass{false};
    std::size_t frames_processed{0};
    std::size_t expectation_mismatches{0};
    std::size_t contract_faults_observed{0};
    std::string first_mismatch;
    ProcessImage final_image;
    BusDiagnostics diagnostics;
};

class ReplayBusBackend final : public IBusBackend {
public:
    explicit ReplayBusBackend(std::vector<ReplayFrame> frames,
                              double counts_per_unit = 1000.0);

    BusExchangeResult exchange(CycleIndex cycle, std::vector<ProcessImage>& images) override;
    const BusDiagnostics& diagnostics() const override;
    const std::vector<ReplayFrame>& frames() const;
    std::size_t frames_consumed() const;
    bool complete() const;

private:
    BusExchangeResult invalid_exchange();

    std::vector<ReplayFrame> frames_;
    double counts_per_unit_{1000.0};
    std::size_t next_frame_{0};
    bool previous_online_{true};
    BusDiagnostics diagnostics_;
};

std::vector<ReplayFrame> load_replay_frames_csv(const std::string& path);
ReplayRunSummary run_replay_file(const std::string& path,
                                 double counts_per_unit = 1000.0,
                                 std::uint32_t stale_rx_limit_cycles = 3);

}  // namespace emrtc
