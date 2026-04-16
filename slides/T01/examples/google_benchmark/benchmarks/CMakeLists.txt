# Build microbenchmarks using Google Benchmark.
#
# Run after building:  ./build/benchmarks/bm_graph
#
# Useful command-line flags (provided by BENCHMARK_MAIN):
#   --benchmark_filter=BFS          Run only benchmarks matching regex
#   --benchmark_format=json         Machine-readable output
#   --benchmark_out=results.json    Save results to file
#   --benchmark_repetitions=5       Run each benchmark 5 times for statistics

add_executable(bm_graph bm_graph.cpp)

# Link against our graph library and Google Benchmark.
# graph_lib provides the include path and C++20 requirement automatically.
# benchmark::benchmark_main provides main() with flag parsing.
target_link_libraries(bm_graph graph_lib benchmark::benchmark_main)
