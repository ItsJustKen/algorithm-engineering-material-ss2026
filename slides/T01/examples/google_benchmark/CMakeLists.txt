# Root CMakeLists.txt — builds tests and benchmarks for the graph library.
#
# This layout mirrors how real C++ projects organize benchmarks:
# a benchmarks/ folder alongside tests/, both as CMake subdirectories.
#
# Quick start:
#     cmake -B build -DCMAKE_BUILD_TYPE=Release
#     cmake --build build
#     ./build/tests/test_graph              # run tests
#     ./build/benchmarks/bm_graph           # run benchmarks
#
# CMAKE_BUILD_TYPE=Release is critical for benchmarks: without it,
# the compiler uses -O0 (no optimization) and results are meaningless.

cmake_minimum_required(VERSION 3.14)
project(google_benchmark_example LANGUAGES CXX)

# ---- Fetch dependencies from GitHub ----
# FetchContent downloads and builds libraries as part of our cmake build.
# No manual installation needed — students just run cmake.
include(FetchContent)

# Google Test — for unit tests.
# We disable what we don't need to speed up the build.
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.16.0
)
# Prevent Google Test from overriding our compiler/linker settings on Windows.
# Harmless on Linux/macOS but avoids subtle link errors on MSVC.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Google Benchmark — for microbenchmarks.
FetchContent_Declare(googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.4
)
# Google Benchmark tries to build its own tests using Google Test.
# We already have Google Test, and we don't need to test the benchmark
# library itself, so disable this to avoid duplicate builds.
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

# ---- Subdirectories ----
# Each subfolder has its own CMakeLists.txt that defines its targets.
# This keeps the root file short and each concern self-contained.
add_subdirectory(src)          # graph_lib (header-only library)
add_subdirectory(tests)        # unit tests (Google Test)
add_subdirectory(benchmarks)   # microbenchmarks (Google Benchmark)
