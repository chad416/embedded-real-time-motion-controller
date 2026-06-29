#include "emrtc/controller.hpp"
#include "emrtc_freertos_task.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include <cstdint>
#include <iostream>

namespace {

struct SmokeContext {
    emrtc::RealTimeController controller;
};

void run_controller_cycle(void* context, std::uint64_t cycle) {
    auto* smoke = static_cast<SmokeContext*>(context);
    if (cycle == 0U) smoke->controller.command({emrtc::CommandType::Enable, 0.0});
    smoke->controller.run_cycles(1);
}

void finish_scheduler(void*) {
    vTaskEndScheduler();
}

}  // namespace

int main() {
    constexpr std::uint64_t kCycleLimit = 25U;
    static SmokeContext context;
    static emrtc::FreeRtosCyclicTaskState state;
    static emrtc::FreeRtosCyclicTaskConfig config{
        pdMS_TO_TICKS(1U),
        kCycleLimit,
        run_controller_cycle,
        finish_scheduler,
        &context,
        &state};
    static StaticTask_t task_control_block;
    static StackType_t task_stack[configMINIMAL_STACK_SIZE * 8U];

    const auto task = xTaskCreateStatic(
        emrtc::emrtc_freertos_cyclic_task,
        "emrtc_cycle",
        sizeof(task_stack) / sizeof(task_stack[0]),
        &config,
        configMAX_PRIORITIES - 2U,
        task_stack,
        &task_control_block);
    if (task == nullptr) {
        std::cerr << "FreeRTOS static task creation failed\n";
        return 1;
    }

    vTaskStartScheduler();
    const auto metrics = context.controller.metrics();
    const bool pass = state.completed &&
                      state.cycles_completed == kCycleLimit &&
                      metrics.cycles.size() == kCycleLimit;
    std::cout << "freertos_cycles=" << state.cycles_completed << '\n'
              << "execution_overruns=" << state.execution_overruns << '\n'
              << "controller_cycles=" << metrics.cycles.size() << '\n'
              << "pass=" << (pass ? "true" : "false") << '\n';
    return pass ? 0 : 1;
}
