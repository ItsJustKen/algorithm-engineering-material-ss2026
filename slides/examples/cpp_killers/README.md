# Example 08: C++ Performance Killers — Five Common Anti-Patterns

Measures the five recurring C++ performance mistakes shown in the T01
"C++ Performance Killers" slides. Each pattern pairs a slow but common
implementation against its fast fix, compiled in Release mode.

## Patterns benchmarked

| # | Pattern | Slow | Fast | Ratio |
|---|---------|------|------|-------|
| 1 | Accidental copies | `auto r` in range-for | `const auto& r` | ~24× |
| 2 | Missing `reserve()` | `push_back` without pre-alloc | `reserve(N)` first | ~2.3× |
| 3 | Erase-in-loop | `v.erase(it)` in loop (O(n²)) | erase-remove idiom (O(n)) | >2000× |
| 4 | AoS vs SoA | 32-byte struct, read one field | SoA: fields contiguous | ~2.3× |
| 5 | Accidental O(n²) | Nested-loop dedup | `sort` + `unique` | ~90× |

Ratios measured on AMD Ryzen 9 9900X, GCC, Release mode (`-O2`/`-O3`).
The erase ratio is N-dependent (O(n²) vs O(n)) and grows with larger N.

## Build and run

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/bm_cpp_killers
```

First run downloads Google Benchmark via CMake FetchContent (~30 s).
Release mode is required — Debug (`-O0`) produces meaningless numbers.

## Run a single pattern

```bash
./build/bm_cpp_killers --benchmark_filter=copy
./build/bm_cpp_killers --benchmark_filter=reserve
./build/bm_cpp_killers --benchmark_filter=erase
./build/bm_cpp_killers --benchmark_filter=aos
./build/bm_cpp_killers --benchmark_filter=dedup
```

## Example output (AMD Ryzen 9 9900X, 48 KB L1d, 32 MB L3)

```
BM_copy_by_value/100000        567 µs   ← heap alloc+memcpy+free per element
BM_copy_by_ref/100000           23 µs   ← reference: zero overhead            ~24×

BM_push_back_no_reserve/10M    19.8 ms  ← ~log2(N) reallocations
BM_push_back_reserve/10M        8.5 ms  ← single allocation upfront            ~2.3×

BM_erase_in_loop/200000       135 ms   ← O(n²): shifts all elements per erase
BM_erase_remove/200000          52 µs   ← O(n): single pass                  >2000×

BM_aos_sum_x/10M               8.3 ms   ← 7/8 of each cache line wasted
BM_soa_sum_x/10M               3.7 ms   ← all `x` values packed              ~2.3×

BM_dedup_naive/100k           348 ms   ← O(n²): scans output per element
BM_dedup_sort_unique/100k       3.8 ms  ← O(n log n): standard library        ~90×
```

## Why these five

They are the most common performance regressions in C++ code that an algorithm
engineer will encounter: unnecessary allocation (1, 2), quadratic algorithms
hiding behind innocent-looking syntax (3, 5), and cache-unfriendly data layout (4).
Each fix is mechanical once you know the pattern.
