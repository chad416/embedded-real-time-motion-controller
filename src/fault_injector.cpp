#include "emrtc/fault_injector.hpp"

#include <algorithm>

namespace emrtc {

void FaultInjector::schedule(FaultInjectionEvent event) {
    events_.push_back(event);
    std::sort(events_.begin(), events_.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.cycle < rhs.cycle;
    });
}

std::vector<FaultInjectionEvent> FaultInjector::events_at(CycleIndex cycle) const {
    std::vector<FaultInjectionEvent> result;
    for (const auto& event : events_) {
        if (event.cycle == cycle) {
            result.push_back(event);
        }
    }
    return result;
}

const std::vector<FaultInjectionEvent>& FaultInjector::events() const {
    return events_;
}

void FaultInjector::clear() {
    events_.clear();
}

}  // namespace emrtc
