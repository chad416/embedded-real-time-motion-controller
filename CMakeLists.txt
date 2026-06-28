cmake_minimum_required(VERSION 3.20)

project(embedded_realtime_motion_controller
    VERSION 0.2.0
    DESCRIPTION "Software-first embedded real-time motion controller simulation and validation bench"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(EMRTC_ENABLE_SOEM "Enable SOEM EtherCAT adapter integration point" OFF)
option(EMRTC_ENABLE_ETHERLAB "Enable EtherLab EtherCAT adapter integration point" OFF)
option(EMRTC_ENABLE_SOCKETCAN "Enable SocketCAN CANopen adapter integration point" OFF)
option(EMRTC_ENABLE_CANOPENNODE "Enable CANopenNode adapter integration point" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(EmrtcAdapterDiscovery)

add_library(emrtc
    src/axis.cpp
    src/adapter_contract.cpp
    src/bus.cpp
    src/controller.cpp
    src/fault_injector.cpp
    src/hardware_adapters.cpp
    src/multi_axis.cpp
    src/process_image_mapping.cpp
    src/replay_adapter.cpp
    src/scenario.cpp
    src/telemetry.cpp
    src/types.cpp
    src/watchdog.cpp)

target_include_directories(emrtc PUBLIC include)

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

add_executable(emrtc_tests tests/test_runner.cpp)
target_link_libraries(emrtc_tests PRIVATE emrtc)

enable_testing()
add_test(NAME emrtc_unit_integration COMMAND emrtc_tests)
set_tests_properties(emrtc_unit_integration PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
