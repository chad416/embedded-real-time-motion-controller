cmake_minimum_required(VERSION 3.20)

project(embedded_realtime_motion_controller
    VERSION 0.2.0
    DESCRIPTION "Software-first embedded real-time motion controller simulation and validation bench"
    LANGUAGES C CXX)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(EMRTC_ENABLE_SOEM "Enable the SOEM EtherCAT backend" OFF)
option(EMRTC_REQUIRE_SOEM "Fail configuration when SOEM v2 is requested but unavailable" OFF)
option(EMRTC_ENABLE_ETHERLAB "Enable EtherLab EtherCAT adapter integration point" OFF)
option(EMRTC_ENABLE_SOCKETCAN "Enable SocketCAN CANopen adapter integration point" OFF)
option(EMRTC_ENABLE_CANOPENNODE "Enable CANopenNode adapter integration point" OFF)
option(EMRTC_ENABLE_FREERTOS_PORT "Build the FreeRTOS cyclic-task port" OFF)
option(EMRTC_REQUIRE_FREERTOS "Fail when the requested FreeRTOS kernel is unavailable" OFF)
set(EMRTC_FREERTOS_KERNEL_PATH "" CACHE PATH "Path to the pinned FreeRTOS-Kernel source tree")
set(EMRTC_FREERTOS_PORT "GCC_POSIX" CACHE STRING "FreeRTOS kernel port used for build validation")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(EmrtcAdapterDiscovery)

add_library(emrtc
    src/axis.cpp
    src/adapter_contract.cpp
    src/adapter_config.cpp
    src/bus.cpp
    src/canopen.cpp
    src/controller.cpp
    src/fault_injector.cpp
    src/hardware_adapters.cpp
    src/json.cpp
    src/multi_axis.cpp
    src/process_image_mapping.cpp
    src/realtime.cpp
    src/replay_adapter.cpp
    src/scenario.cpp
    src/telemetry.cpp
    src/types.cpp
    src/watchdog.cpp)

target_include_directories(emrtc PUBLIC include)
target_link_libraries(emrtc PRIVATE Threads::Threads)

if(EMRTC_SOEM_INCLUDE_DIRS)
    target_include_directories(emrtc PRIVATE ${EMRTC_SOEM_INCLUDE_DIRS})
endif()

if(EMRTC_ETHERLAB_INCLUDE_DIRS)
    target_include_directories(emrtc PRIVATE ${EMRTC_ETHERLAB_INCLUDE_DIRS})
endif()

if(EMRTC_SOCKETCAN_INCLUDE_DIRS)
    target_include_directories(emrtc PRIVATE ${EMRTC_SOCKETCAN_INCLUDE_DIRS})
endif()

if(EMRTC_CANOPENNODE_INCLUDE_DIRS)
    target_include_directories(emrtc PRIVATE ${EMRTC_CANOPENNODE_INCLUDE_DIRS})
endif()

target_compile_definitions(emrtc PUBLIC
    $<$<BOOL:${EMRTC_ENABLE_SOEM}>:EMRTC_ENABLE_SOEM=1>
    $<$<BOOL:${EMRTC_ENABLE_ETHERLAB}>:EMRTC_ENABLE_ETHERLAB=1>
    $<$<BOOL:${EMRTC_ENABLE_SOCKETCAN}>:EMRTC_ENABLE_SOCKETCAN=1>
    $<$<BOOL:${EMRTC_ENABLE_CANOPENNODE}>:EMRTC_ENABLE_CANOPENNODE=1>
    $<$<BOOL:${EMRTC_HAS_SOEM}>:EMRTC_HAS_SOEM=1>
    $<$<BOOL:${EMRTC_HAS_ETHERLAB}>:EMRTC_HAS_ETHERLAB=1>
    $<$<BOOL:${EMRTC_HAS_SOCKETCAN}>:EMRTC_HAS_SOCKETCAN=1>
    $<$<BOOL:${EMRTC_HAS_CANOPENNODE}>:EMRTC_HAS_CANOPENNODE=1>)

if(EMRTC_SOEM_LIBRARIES)
    target_link_libraries(emrtc PRIVATE ${EMRTC_SOEM_LIBRARIES})
endif()

if(EMRTC_ETHERLAB_LIBRARIES)
    target_link_libraries(emrtc PRIVATE ${EMRTC_ETHERLAB_LIBRARIES})
endif()

if(EMRTC_CANOPENNODE_LIBRARIES)
    target_link_libraries(emrtc PRIVATE ${EMRTC_CANOPENNODE_LIBRARIES})
endif()

if(MSVC)
    target_compile_options(emrtc PRIVATE /W4 /permissive-)
else()
    target_compile_options(emrtc PRIVATE -Wall -Wextra -Wpedantic)
endif()

message(STATUS "EMRTC adapter flags:")
message(STATUS "  EMRTC_ENABLE_SOEM=${EMRTC_ENABLE_SOEM}")
message(STATUS "  EMRTC_REQUIRE_SOEM=${EMRTC_REQUIRE_SOEM}")
message(STATUS "  EMRTC_ENABLE_ETHERLAB=${EMRTC_ENABLE_ETHERLAB}")
message(STATUS "  EMRTC_ENABLE_SOCKETCAN=${EMRTC_ENABLE_SOCKETCAN}")
message(STATUS "  EMRTC_ENABLE_CANOPENNODE=${EMRTC_ENABLE_CANOPENNODE}")
message(STATUS "  EMRTC_HAS_SOEM=${EMRTC_HAS_SOEM}")
message(STATUS "  EMRTC_HAS_ETHERLAB=${EMRTC_HAS_ETHERLAB}")
message(STATUS "  EMRTC_HAS_SOCKETCAN=${EMRTC_HAS_SOCKETCAN}")
message(STATUS "  EMRTC_HAS_CANOPENNODE=${EMRTC_HAS_CANOPENNODE}")

add_executable(emrtc_sim apps/simulator_main.cpp)
target_link_libraries(emrtc_sim PRIVATE emrtc)

add_executable(emrtc_benchmark apps/benchmark_main.cpp)
target_link_libraries(emrtc_benchmark PRIVATE emrtc)

add_executable(emrtc_cli apps/diagnostic_cli_main.cpp)
target_link_libraries(emrtc_cli PRIVATE emrtc)

add_executable(emrtc_realtime_probe apps/realtime_probe_main.cpp)
target_link_libraries(emrtc_realtime_probe PRIVATE emrtc)

add_executable(emrtc_tests tests/test_runner.cpp)
target_link_libraries(emrtc_tests PRIVATE emrtc)

enable_testing()
add_test(NAME emrtc_unit_integration COMMAND emrtc_tests)
set_tests_properties(emrtc_unit_integration PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

if(EMRTC_ENABLE_FREERTOS_PORT)
    if(EXISTS "${EMRTC_FREERTOS_KERNEL_PATH}/CMakeLists.txt")
        add_library(freertos_config INTERFACE)
        target_include_directories(freertos_config SYSTEM INTERFACE
            "${CMAKE_CURRENT_SOURCE_DIR}/ports/freertos")
        set(FREERTOS_PORT "${EMRTC_FREERTOS_PORT}" CACHE STRING "" FORCE)
        set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
        add_subdirectory("${EMRTC_FREERTOS_KERNEL_PATH}"
                         "${CMAKE_BINARY_DIR}/freertos-kernel")

        add_library(emrtc_freertos_port
            ports/freertos/emrtc_freertos_task.cpp)
        target_include_directories(emrtc_freertos_port PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/ports/freertos")
        target_link_libraries(emrtc_freertos_port PUBLIC emrtc freertos_kernel)
        target_compile_definitions(emrtc_freertos_port PUBLIC EMRTC_HAS_FREERTOS=1)

        add_executable(emrtc_freertos_smoke apps/freertos_smoke_main.cpp)
        target_link_libraries(emrtc_freertos_smoke PRIVATE emrtc_freertos_port)
        add_test(NAME emrtc_freertos_cyclic_smoke COMMAND emrtc_freertos_smoke)
        set_tests_properties(emrtc_freertos_cyclic_smoke PROPERTIES TIMEOUT 20)
        message(STATUS "FreeRTOS cyclic-task port enabled: ${EMRTC_FREERTOS_PORT}")
    elseif(EMRTC_REQUIRE_FREERTOS)
        message(FATAL_ERROR "FreeRTOS port required but EMRTC_FREERTOS_KERNEL_PATH is invalid")
    else()
        message(STATUS "FreeRTOS port requested without a kernel source tree; port target omitted")
    endif()
endif()
