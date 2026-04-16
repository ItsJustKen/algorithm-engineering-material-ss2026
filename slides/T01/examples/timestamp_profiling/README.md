# Example 01: Timestamp Profiling with Basic Logging

The first performance tool to reach for is often just logging. Add a few
phase markers, let the timestamps tell you where the time goes, and avoid
turning the output into noise.

## What this shows

- **Basic logs are often enough.** A few start/end markers already tell you
  which phase is slow because the logger prints timestamps automatically.
- **The bottleneck is not where you expect.** You might assume the NP-hard
  CP-SAT solver dominates, but the logs reveal that *clique enumeration*
  is the real time sink — before the solver even starts.
- **Structural statistics point to the root cause.** Seeing "800,000 cliques"
  in the log immediately explains why everything downstream is slow.  A
  400-node graph with max degree 22 shouldn't need that many constraints.
- **The fix is algorithmic, not micro-optimization.** No amount of profiling
  the CP-SAT call will help when the real problem is enumerating hundreds
  of thousands of redundant constraints.

## Run

```bash
pip install networkx ortools
python slow_solver.py
```

## Example output (AMD Ryzen 9 9900X)

```
2026-04-13 19:35:35,175 INFO Graph: nodes=400  edges=3800
2026-04-13 19:35:35,175 INFO Running greedy coloring for upper bound
2026-04-13 19:35:35,176 INFO Greedy coloring done: 14 colors
2026-04-13 19:35:35,176 INFO Enumerating all cliques...
2026-04-13 19:35:41,413 INFO Cliques done: count=800987  max_size=14
2026-04-13 19:35:41,414 INFO Building CP-SAT model with all_different on each clique
2026-04-13 19:35:43,006 INFO Model built: 400 variables, 800987 constraints
2026-04-13 19:35:43,006 INFO Solving...
2026-04-13 19:36:27,960 INFO Solve done: status=UNKNOWN  wall_time=44.88s
```

Reading top to bottom: greedy coloring is instant.  Then clique enumeration
takes **6 seconds** and produces 800,000 cliques — that number alone is a
red flag.  The model building and solve are also slow because they inherit
the bloated constraint set.

Without the logs, you might have stared at the CP-SAT call wondering why
it is slow.  The logs show the problem started much earlier.

## The pattern

```python
import logging

logging.basicConfig(level=logging.INFO,
                    format="%(asctime)s %(levelname)s %(message)s")
log = logging.getLogger(__name__)

log.info("Starting phase X")
result = do_phase_x()
log.info("Phase X done: items=%d", len(result))
```

This is the first thing to try before reaching for a profiler. It works in
any language, requires almost no setup, and often narrows the search space
enough to tell you what tool to use next.

## Practical advice

- Log **phase boundaries**, not every tiny operation
- Log a few **structural statistics** so the run is interpretable
- Keep messages stable and easy to scan
- Once you know the slow phase, switch to a profiler for finer detail

## When to use something more powerful

Timestamp logging tells you **which phase** is slow but not **why**.
Once you know the slow phase, use:

- **Scalene** (example 02) to see whether the bottleneck is Python overhead,
  native code, or memory
- **cProfile** (example 03) to count function calls and find hot functions
- **perf** for CPU-level analysis (cache misses, branch mispredictions)
