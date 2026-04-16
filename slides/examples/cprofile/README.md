# Example 03: cProfile — Exact Call Counts and Call Paths

Use cProfile when you need **exact call counts** and **call-path structure**.
Unlike sampling profilers, cProfile traces every function call, so counts are
precise — but timing can be distorted for functions with very high call rates.

## What this shows

- **Call counts reveal algorithmic bugs.** `_extend` called 1.7 million times and
  `add_all_different` called 206k times tell you the problem is combinatorial
  explosion, not slow per-call performance.
- **Three usage patterns**, from simplest to most flexible:
  1. Command-line (`python -m cProfile`)
  2. Context manager (`cProfile.Profile()`)
  3. Post-analysis with `pstats`
- **CLI profiles everything including imports**, which adds noise. The context
  manager lets you profile only the section you care about.
- **`pstats` enables deeper investigation** — re-sort by different criteria,
  filter by function name, and trace caller/callee relationships.

## Run

```bash
pip install networkx ortools

# Default: demonstrates context manager + pstats analysis
python profile_solver.py

# CLI mode: profiles the entire script (including imports)
python -m cProfile -s tottime profile_solver.py --cli-demo
```

## Example output (AMD Ryzen 9 9900X, CPython 3.12)

### Context manager (targeted profiling)

```
   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
   206534    0.385    0.000    0.385    0.000 cp_model.py:537(add_all_different)
1752679/206535    0.253    0.000    0.260    0.000 profile_solver.py:39(_extend)
   206535    0.014    0.000    0.274    0.000 profile_solver.py:34(enumerate_all_cliques)
```

### pstats caller analysis

```
--- Callers of '_extend' ---
profile_solver.py:39(_extend) <-  206535  profile_solver.py:34(enumerate_all_cliques)
                                 1546144  profile_solver.py:39(_extend)  [recursive!]
```

The caller analysis shows `_extend` is called 206k times from `enumerate_all_cliques`
and 1.5 million times recursively from itself — a clear sign of combinatorial blowup.

## Reading the output

| Column | Meaning |
|--------|---------|
| `ncalls` | Number of calls (`total/primitive` for recursive functions) |
| `tottime` | Time spent in this function only (excluding callees) |
| `cumtime` | Time spent in this function including callees |
| `percall` | Per-call time (tottime or cumtime divided by ncalls) |

**Sort keys:** `-s tottime` finds the hot function; `-s cumtime` finds the
top-level bottleneck; `-s ncalls` finds unexpectedly frequent calls.

## When to use cProfile vs. other tools

| Need | Tool |
|------|------|
| Exact call counts, call graph structure | **cProfile** |
| Realistic timing with low overhead | Scalene (sampling) |
| Line-level hotspots, memory, Python vs native | Scalene |
| CPU-level details (cache misses, branch mispredicts) | perf (C++/native) |

## Practical tips

- **Trust call counts, cross-check timing.** cProfile's tracing overhead can
  distort absolute timings, especially for functions called millions of times.
- **Use the context manager** to avoid profiling imports and argument parsing.
- **Save profiles with `dump_stats()`** so you can re-analyze without re-running.
- **`print_callers()`/`print_callees()`** are the killer feature for understanding
  *why* a function is called so often.
