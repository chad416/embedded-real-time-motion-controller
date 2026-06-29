#include "emrtc_freertos_task.hpp"

namespace emrtc {

void emrtc_freertos_cyclic_task(void* parameter) {
    auto* config = static_cast<FreeRtosCyclicTaskConfig*>(parameter);
    configASSERT(config != nullptr);
    configASSERT(config->state != nullptr);
    configASSERT(config->period_ticks > 0);
    configASSERT(config->cycle_limit > 0);
    configASSERT(config->cycle_function != nullptr);

    TickType_t last_wake = xTaskGetTickCount();
    for (std::uint64_t cycle = 0; cycle < config->cycle_limit; ++cycle) {
        const TickType_t start = xTaskGetTickCount();
        config->cycle_function(config->context, cycle);
        const TickType_t elapsed = xTaskGetTickCount() - start;
        if (elapsed >= config->period_ticks) {
            ++config->state->execution_overruns;
        }
        ++config->state->cycles_completed;
        (void)xTaskDelayUntil(&last_wake, config->period_ticks);
    }

    config->state->completed = true;
    if (config->completion_function != nullptr) {
        config->completion_function(config->context);
    }
    vTaskDelete(nullptr);
}

}  // namespace emrtc
