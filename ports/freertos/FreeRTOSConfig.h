#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define configCPU_CLOCK_HZ                         ((unsigned long)20000000)
#define configTICK_RATE_HZ                         1000U
#define configUSE_PREEMPTION                       1
#define configUSE_TIME_SLICING                     0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION    0
#define configUSE_TICKLESS_IDLE                    0
#define configMAX_PRIORITIES                       8U
#define configMINIMAL_STACK_SIZE                   256U
#define configMAX_TASK_NAME_LEN                    24U
#define configTICK_TYPE_WIDTH_IN_BITS              TICK_TYPE_WIDTH_64_BITS
#define configIDLE_SHOULD_YIELD                    1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES      1U
#define configQUEUE_REGISTRY_SIZE                  0U
#define configENABLE_BACKWARD_COMPATIBILITY        1
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS    0
#define configSTACK_DEPTH_TYPE                     size_t
#define configMESSAGE_BUFFER_LENGTH_TYPE           size_t
#define configUSE_NEWLIB_REENTRANT                 0
#define configUSE_MINI_LIST_ITEM                   0

#define configUSE_TIMERS                           0
#define configSUPPORT_STATIC_ALLOCATION            1
#define configSUPPORT_DYNAMIC_ALLOCATION           1
#define configTOTAL_HEAP_SIZE                      (64U * 1024U)
#define configAPPLICATION_ALLOCATED_HEAP           0
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP  0

#define configKERNEL_INTERRUPT_PRIORITY            0U
#define configMAX_SYSCALL_INTERRUPT_PRIORITY       0U
#define configMAX_API_CALL_INTERRUPT_PRIORITY      0U

#define configUSE_IDLE_HOOK                        0
#define configUSE_TICK_HOOK                        0
#define configUSE_MALLOC_FAILED_HOOK               0
#define configUSE_DAEMON_TASK_STARTUP_HOOK         0
#define configCHECK_FOR_STACK_OVERFLOW             0
#define configGENERATE_RUN_TIME_STATS              0
#define configUSE_TRACE_FACILITY                   0
#define configUSE_STATS_FORMATTING_FUNCTIONS       0
#define configKERNEL_PROVIDED_STATIC_MEMORY        1

#define configUSE_TASK_NOTIFICATIONS               1
#define configUSE_MUTEXES                          1
#define configUSE_RECURSIVE_MUTEXES                0
#define configUSE_COUNTING_SEMAPHORES              1
#define configUSE_QUEUE_SETS                       0
#define configUSE_APPLICATION_TASK_TAG             0
#define configUSE_POSIX_ERRNO                      0

#define INCLUDE_vTaskPrioritySet                   1
#define INCLUDE_uxTaskPriorityGet                  1
#define INCLUDE_vTaskDelete                        1
#define INCLUDE_vTaskSuspend                       1
#define INCLUDE_vTaskDelayUntil                    1
#define INCLUDE_vTaskDelay                         1
#define INCLUDE_xTaskGetSchedulerState             1
#define INCLUDE_xTaskGetCurrentTaskHandle          1
#define INCLUDE_uxTaskGetStackHighWaterMark        1
#define INCLUDE_xTaskGetIdleTaskHandle             0
#define INCLUDE_eTaskGetState                      0
#define INCLUDE_xTimerPendFunctionCall             0
#define INCLUDE_xTaskAbortDelay                    0
#define INCLUDE_xTaskGetHandle                     0
#define INCLUDE_xTaskResumeFromISR                 1

#define configASSERT(value) assert(value)
