#pragma once

#include "FreeRTOS.h"
#include "task.h"

#include <cstdint>

namespace emrtc {

using FreeRtosCycleFunction = void (*)(void* context, std::uint64_t cycle);
using FreeRtosCompletionFunction = void (*)(void* context);

struct FreeRtosCyclicTaskState {
    volatile std::uint64_t cycles_completed{0};
    volatile std::uint64_t execution_overruns{0};
    volatile bool completed{false};
};

struct FreeRtosCyclicTaskConfig {
    TickType_t period_ticks{1};
    std::uint64_t cycle_limit{0};
    FreeRtosCycleFunction cycle_function{nullptr};
    FreeRtosCompletionFunction completion_function{nullptr};
    void* context{nullptr};
    FreeRtosCyclicTaskState* state{nullptr};
};

void emrtc_freertos_cyclic_task(void* parameter);

}  // namespace emrtc
