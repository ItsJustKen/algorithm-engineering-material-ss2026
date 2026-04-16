# Example 06: Google Benchmark — Graph Data Structure Microbenchmarks

Demonstrates how to set up [Google Benchmark](https://github.com/google/benchmark)
in a C++ project with a `benchmarks/` folder alongside `tests/` — the same way
real performance-critical projects are organized.

## What this shows

- **Project structure**: `src/` (library), `tests/` (Google Test), `benchmarks/` (Google Benchmark)
- **FetchContent**: Both libraries downloaded automatically by CMake — no manual installation
- **Data layout matters**: AdjacencyList vs. CSR — same graph, same algorithms, different performance
- **Google Benchmark API**: `State`, `DoNotOptimize`, `Range`, `SetItemsProcessed`

## The benchmark: AdjacencyList vs. CSR

Both representations store the same graph and expose the same API (`neighbors(v)`,
`degree(v)`), but their memory layout differs fundamentally:

| | AdjacencyList | CSR |
|---|---|---|
| Storage | `vector<vector<int>>` | Two flat arrays: `offsets[]` + `targets[]` |
| Memory layout | Each vertex's neighbors in a separate heap allocation | All neighbors packed contiguously |
| Cache behavior | Pointer chasing between scattered blocks | Linear memory walk |
| Mutability | Easy to add/remove edges | Immutable after construction |

Three benchmarks measure the impact:

1. **Iterate all neighbors** — the core loop of Dijkstra, BFS, PageRank, etc.
2. **BFS traversal** — a real algorithm showing the compound effect
3. **Random vertex access** — simulates random walks, Monte Carlo methods

## Project layout

```
google_benchmark/
├── CMakeLists.txt              # Root: fetches dependencies, adds subdirectories
├── src/
│   ├── CMakeLists.txt          # Defines graph_lib (INTERFACE library target)
│   └── graph/
│       ├── graph.h             # AdjacencyList + CSR implementations
│       └── graph_gen.h         # Deterministic random graph generator
├── benchmarks/
│   ├── CMakeLists.txt          # Benchmark target, links graph_lib
│   └── bm_graph.cpp            # Microbenchmarks
├── tests/
│   ├── CMakeLists.txt          # Test target, links graph_lib
│   └── test_graph.cpp          # Correctness tests
└── README.md
```

`src/` defines a proper CMake library target (`graph_lib`). Benchmarks and tests
link against it, which provides include paths and compile settings (C++20)
automatically — no relative-path `#include` hacks needed.

## Build and run

```bash
# Configure — Release mode is critical (Debug uses -O0, results are meaningless)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build (first run downloads Google Test + Google Benchmark, takes ~1 min)
cmake --build build

# Run tests first — a fast wrong answer is useless
./build/tests/test_graph

# Run benchmarks
./build/benchmarks/bm_graph
```

## Example output (AMD Ryzen 9 9900X, 48 KB L1d, 32 MB L3)

Abbreviated to highlight the pattern (N=1k vs N=1M):

```
BM_iterate_neighbors_adjlist/1000          1459 ns    items/s=6.87G
BM_iterate_neighbors_adjlist/1000000   14974875 ns    items/s=670M     ← 10× throughput drop
BM_iterate_neighbors_csr/1000              1480 ns    items/s=6.77G
BM_iterate_neighbors_csr/1000000        8414209 ns    items/s=1.19G    ← only 6× drop

BM_bfs_adjlist/1000000                126578169 ns
BM_bfs_csr/1000000                     69913862 ns                     ← 1.8× faster

BM_random_access_adjlist/1000000         19548 ns    items/s=513M
BM_random_access_csr/1000000             11157 ns    items/s=899M      ← 1.8× faster
```

At N=1000 both fit in cache and perform similarly. At N=1M, CSR wins
decisively: **1.8× on iteration**, **1.8× on BFS**, **1.8× on random access**.

## Useful flags

```bash
# Run only benchmarks matching a regex
./build/benchmarks/bm_graph --benchmark_filter=bfs

# JSON output for scripts/plotting
./build/benchmarks/bm_graph --benchmark_format=json --benchmark_out=results.json

# Multiple repetitions for statistics (mean, median, stddev)
./build/benchmarks/bm_graph --benchmark_repetitions=5
```

## Writing benchmarks with Google Benchmark

### Minimal benchmark function

```cpp
#include <benchmark/benchmark.h>

static void BM_my_function(benchmark::State& state) {
    // Setup: not measured
    auto data = prepare_data(state.range(0));

    // Measured loop: Google Benchmark decides iteration count
    for (auto _ : state) {
        auto result = do_work(data);
        benchmark::DoNotOptimize(result);  // prevent dead-code elimination
    }
}

BENCHMARK(BM_my_function)->Arg(1000)->Arg(10000);
```

### DoNotOptimize

```cpp
auto result = bfs(graph, 0);
benchmark::DoNotOptimize(result);
```

Without this, the compiler sees that `result` is unused and eliminates the
entire function call — you'd be benchmarking an empty loop.
Use it on every result that isn't otherwise consumed.

### ClobberMemory

```cpp
auto original = prepare_data(state.range(0));

for (auto _ : state) {
    state.PauseTiming();
    auto data = original;                // restore identical input every round
    state.ResumeTiming();

    modify_data_structure(data);
    benchmark::ClobberMemory();  // force memory writes to complete
}
```

Use `ClobberMemory()` when modifying data in-place and the compiler might
optimize away stores it considers "dead." `DoNotOptimize` protects values;
`ClobberMemory` protects memory side effects.
If the benchmark mutates its input, reset identical input every iteration;
pause timing around that reset if you want to measure only the algorithm itself.

### Parameterized benchmarks

```cpp
// Single parameter via Range (powers of 10):
BENCHMARK(BM_sort)->RangeMultiplier(10)->Range(100, 1000000);

// Multiple parameters via Args:
BENCHMARK(BM_matrix)->Args({100, 100})->Args({1000, 10});

// Access parameters in the function:
int rows = state.range(0);
int cols = state.range(1);
```

### Reporting throughput

```cpp
// Report items processed per second (shows as "items/s" in output)
state.SetItemsProcessed(state.iterations() * n);

// Report bytes processed per second (shows as "bytes/s")
state.SetBytesProcessed(state.iterations() * n * sizeof(int));
```

This makes benchmarks with different N directly comparable by throughput
rather than raw time.

### Setup vs. measured code

Only code inside `for (auto _ : state)` is measured. Expensive setup
(graph construction, data generation) goes before the loop and is excluded
from timing. Google Benchmark automatically determines how many iterations
to run for statistical stability.

### CMake integration

Google Benchmark is fetched automatically via `FetchContent` — no system
installation needed. The key lines in the root `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.9.4)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)
```

Your library code in `src/` is defined as an INTERFACE target (`src/CMakeLists.txt`):

```cmake
add_library(graph_lib INTERFACE)
target_include_directories(graph_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(graph_lib INTERFACE cxx_std_20)
```

Then in `benchmarks/CMakeLists.txt`, link against both:

```cmake
add_executable(bm_graph bm_graph.cpp)
target_link_libraries(bm_graph graph_lib benchmark::benchmark_main)
```

`graph_lib` provides include paths and compile settings.
`benchmark_main` provides `main()` with built-in flag parsing.
Use `benchmark::benchmark` instead if you need a custom `main()`.
