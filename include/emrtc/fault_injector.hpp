#pragma once

#include "emrtc/types.hpp"

#include <vector>

namespace emrtc {

struct FaultInjectionEvent {
    CycleIndex cycle{0};
    FaultKind kind{FaultKind::MissedCycle};
    double magnitude{1.0};
    std::uint32_t duration_cycles{1};
};

class FaultInjector {
public:
    void schedule(FaultInjectionEvent event);
    std::vector<FaultInjectionEvent> events_at(CycleIndex cycle) const;
    const std::vector<FaultInjectionEvent>& events() const;
    void clear();

private:
    std::vector<FaultInjectionEvent> events_;
};

}  // namespace emrtc
