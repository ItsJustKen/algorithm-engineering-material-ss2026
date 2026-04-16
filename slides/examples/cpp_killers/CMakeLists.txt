# Root CMakeLists.txt — builds the C++ performance killers benchmark.
#
# Benchmarks the five patterns from the "C++ Performance Killers" slide section.
# Each pattern compares a slow but common implementation to its fast fix.
#
# Quick start:
#     cmake -B build -DCMAKE_BUILD_TYPE=Release
#     cmake --build build
#     ./build/bm_cpp_killers
#
# Release mode is critical: Debug (-O0) produces meaningless results.

cmake_minimum_required(VERSION 3.14)
project(cpp_killers_benchmarks LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.4
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(bm_cpp_killers benchmarks/bm_cpp_killers.cpp)
target_link_libraries(bm_cpp_killers benchmark::benchmark_main)
