# Exercise 03: TSP 2-Opt

This exercise implements the 2-opt heuristic for the Traveling Salesman Problem
using several move-selection strategies, in both Python and C++.

## Updates

Post-release fixes. Only changes that affect your workflow are listed here;
see `git log` for the full history.

- **2026-04-16** — Python variants (`first_improvement_two_opt`,
  `full_scan_two_opt`, `best_improvement_two_opt`) now take an optional
  `timeout: float = 10.0` parameter, matching the C++ API. If you already
  started: add the parameter to your signature and check a deadline
  (`time.perf_counter()`) in your outer loop — coarse-grained checks after
  each restart or full sweep are enough. Without this, `benchmark.py` could
  run for hours on large `n`. See issue #1.

## Project Structure

```
exercise03/
├── pyproject.toml          # Build configuration
├── CMakeLists.txt          # CMake build for the C++ pybind11 module
├── src/tsp_two_opt/
│   ├── __init__.py         # Package init (imports all implementations)
│   ├── first_improvement.py # Part a: first-improvement 2-opt in Python
│   ├── full_scan.py        # Part a: full-scan 2-opt in Python
│   ├── best_improvement.py # Part a: best-improvement 2-opt in Python
│   └── _core.cpp           # Parts b+c: C++ implementations via pybind11
└── benchmark.py            # Generates instances, runs all implementations
```

## Setup

### Native (recommended)

Prerequisites: Python 3.11+, a C++ compiler with C++20 support, CMake 3.20+.

```bash
cd exercise03
pip install -e .       # Installs the package + builds C++ module
python benchmark.py    # Run benchmarks
```

After modifying `_core.cpp`, re-run `pip install -e .` to rebuild.

### Docker (alternative)

A Dockerfile is provided if you don't have a local toolchain.

```bash
docker build -t ae26-tsp .
docker run --rm -it -v $(pwd):/app ae26-tsp /bin/bash
```

Inside the container:

```bash
pip install .          # Build the C++ module
python benchmark.py    # Run benchmarks
```

After changing `_core.cpp`, re-run `pip install .` to rebuild.
Your edits on the host are immediately visible inside the container.

## Testing

Run the test suite to check your implementations:

```bash
python -m pytest tests/ -v
```

The tests verify that each implementation returns a valid tour, does not make
the tour worse, and produces a 2-optimal result (no improving 2-opt move
exists). The C++ tests also check that results match the Python implementations
when given the same initial tour.

## Benchmark

The benchmark generates random uniform instances on the fly and runs all
available implementations on the same points. Each run has a **10-second
timeout**, so slow variants on large instances will not block the script.

```bash
python benchmark.py              # run all sizes (100, 500, 1000, 5000, 20000)
python benchmark.py 100 500      # run only specific sizes
```

For each instance size, the benchmark runs:
- **Python variants**: first-improvement, best-improvement, full-scan
- **C++ variants**: first-improvement, best-improvement, full-scan
- **Parallel C++**: 1, 2, 4, and 8 threads

and prints a summary table comparing tour length and runtime.

## Function Signatures

All single-run functions take a list of `(x, y)` tuples, an `initial_tour`
(a permutation of `0..n-1`), and an optional `timeout` in seconds, and
return the improved tour. The benchmark generates the initial tour in
Python and passes it to every variant, so all start from the same
permutation and results are directly comparable.

```python
# Part a: Python variants (all take an optional timeout in seconds)
first_improvement_two_opt(points, initial_tour, timeout=10.0) -> list[int]
full_scan_two_opt(points, initial_tour, timeout=10.0) -> list[int]
best_improvement_two_opt(points, initial_tour, timeout=10.0) -> list[int]

# Part b: C++ variants (all take an optional timeout in seconds)
cpp_first_improvement(points, initial_tour, timeout=10.0) -> list[int]
cpp_full_scan(points, initial_tour, timeout=10.0) -> list[int]
cpp_best_improvement(points, initial_tour, timeout=10.0) -> list[int]

# Part c: parallel 2-opt (generates its own random tours internally)
parallel_two_opt(points, num_threads=4, base_seed=0, timeout=10.0) -> list[int]
```

When the timeout is reached, return the best tour found so far. Coarse-grained
checks are fine: compare `time.perf_counter()` (Python) or `Clock::now()` (C++)
against a precomputed deadline after each restart or full sweep, not inside
the innermost loop.

## Build Troubleshooting

**C++ changes not taking effect?**
The `-e` (editable) flag only live-updates Python source files. The compiled
C++ module (`.so` / `.pyd`) requires a rebuild: run `pip install -e .` again
after editing `_core.cpp`. If the old behavior persists, try
`pip install -e . --no-build-isolation` or delete the `_skbuild/` directory.

**Windows: MinGW build fails?**
scikit-build-core does not reliably work with MinGW
([scikit-build-core#900](https://github.com/scikit-build/scikit-build-core/issues/900)).
Install [Visual Studio Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/)
(the "Desktop development with C++" workload) or use the Docker fallback
described above.

**CMake error: `pybind11Config.cmake` not found?**
This means pybind11's CMake files are missing from your environment
([pybind11#3445](https://github.com/pybind/pybind11/issues/3445)). Fix:

```bash
pip install pybind11
pip install -e .
```

**Using `uv` instead of `pip`?**
scikit-build-core's editable mode is currently incompatible with `uv`
([uv#14383](https://github.com/astral-sh/uv/issues/14383)). Use standard
`pip` for this exercise.

**`ImportError` when importing C++ functions?**
If you renamed `_core.cpp` or changed the `PYBIND11_MODULE(_core, m)` macro,
the module name must match the CMake target name in `CMakeLists.txt`. A
mismatch causes a silent import failure.

## About pybind11

[pybind11](https://pybind11.readthedocs.io/) lets you call C++ functions
directly from Python. In `_core.cpp`, the `PYBIND11_MODULE` block at the bottom
registers your C++ functions so they appear as regular Python functions in the
`tsp_two_opt` package. The build system (CMake + scikit-build-core) compiles the
C++ code into a shared library that Python imports automatically. You only need
to write the algorithm in C++ — the bindings and build plumbing are already set up.

## Hints for Part c (Parallel)

- Timeout checks can be coarse-grained. For first-improvement, check the
  deadline after each restart round. For full-scan and best-improvement,
  check it after each full scan over all candidate pairs. If the timeout is
  reached, return the current tour; there is no need to check inside every
  inner-loop iteration.
- Each thread runs one 2-opt search with a different seed (e.g., `base_seed + t`).
- The threads are pure C++ and do not touch Python objects, so they run
  truly in parallel without any special handling.
- After all threads finish, return the tour with the shortest length.
- Reject invalid inputs such as `num_threads < 1` explicitly.

Skeleton for launching threads:

```cpp
std::vector<std::thread> threads;
for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
        // run 2-opt with seed = base_seed + t
    });
}
for (auto& th : threads) th.join();
```

You will need to store each thread's result and find the best tour after joining.
