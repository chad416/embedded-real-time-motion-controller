cmake_minimum_required(VERSION 3.20)

project(embedded_realtime_motion_controller
    VERSION 0.1.0
    DESCRIPTION "Software-first embedded real-time motion controller simulation and validation bench"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(emrtc
    src/axis.cpp
    src/bus.cpp
    src/controller.cpp
    src/fault_injector.cpp
    src/telemetry.cpp
    src/types.cpp
    src/watchdog.cpp)

target_include_directories(emrtc PUBLIC include)

if(MSVC)
    target_compile_options(emrtc PRIVATE /W4 /permissive-)
else()
    target_compile_options(emrtc PRIVATE -Wall -Wextra -Wpedantic)
endif()

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
