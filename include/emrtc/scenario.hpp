#pragma once

#include "emrtc/controller.hpp"

#include <string>
#include <vector>

namespace emrtc {

struct ScenarioCommandEvent {
    CycleIndex cycle{0};
    AxisCommand command;
};

struct ScenarioExpectation {
    std::uint64_t missed_cycles_min{0};
    std::uint64_t deadline_overruns_min{0};
    std::uint64_t packet_loss_count_min{0};
    std::uint64_t heartbeat_timeout_count_min{0};
    std::uint64_t cable_disconnect_events_min{0};
    FaultCode final_fault{FaultCode::None};
};

struct ScenarioDefinition {
    std::string name{"unnamed scenario"};
    ControllerConfig config;
    std::uint64_t run_cycles{2500};
    std::vector<FaultInjectionEvent> faults;
    std::vector<ScenarioCommandEvent> commands;
    ScenarioExpectation expected;
};

ScenarioDefinition load_scenario_file(const std::string& path);
RuntimeMetrics run_scenario_definition(const ScenarioDefinition& scenario);
RuntimeMetrics run_scenario_file(const std::string& path);
std::vector<std::string> list_scenario_files(const std::string& directory);

}  // namespace emrtc
