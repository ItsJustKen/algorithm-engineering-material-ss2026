"""
Benchmark script for the "Python Performance Killers" slides (T01).

Reproduces all the before/after comparisons from the slides:
  1. set vs list membership
  2. string concatenation (+=  vs join)
  3. builtin sum() vs manual loop
  4. lru_cache memoization
  5. NumPy matmul vs Python loops
  6. data containers (tuple / class / dataclass / NamedTuple / Pydantic)

Run:
    python benchmark_python_killers.py

Requires: numpy, pydantic  (pip install numpy pydantic)
"""

import sys
import timeit


def header(title: str) -> None:
    print(f"\n{'=' * 60}")
    print(f"  {title}")
    print(f"{'=' * 60}")


def report(label_slow: str, t_slow: float, label_fast: str, t_fast: float) -> None:
    speedup = t_slow / t_fast if t_fast > 0 else float("inf")
    print(f"  {label_slow:30s}: {t_slow * 1000:10.3f} ms")
    print(f"  {label_fast:30s}: {t_fast * 1000:10.3f} ms")
    print(f"  {'speedup':30s}: {speedup:10.0f}x")


# ── 1. set vs list membership ─────────────────────────────────────────────

def bench_membership():
    header("1. set vs list membership")

    setup = """
data_list = list(range(10_000))
data_set  = set(range(10_000))
queries   = [5_000, 9_999, 10_001, 42, 7_777] * 200
"""
    stmt_list = "sum(1 for q in queries if q in data_list)"
    stmt_set  = "sum(1 for q in queries if q in data_set)"

    t_list = min(timeit.repeat(stmt_list, setup=setup, number=100, repeat=5)) / 100
    t_set  = min(timeit.repeat(stmt_set,  setup=setup, number=100, repeat=5)) / 100

    report("list (linear scan)", t_list, "set (hash lookup)", t_set)


# ── 2. string concatenation ───────────────────────────────────────────────

def bench_string_concat():
    header("2. string concatenation")

    setup = "parts = ['hello'] * 10_000"
    stmt_slow = """
s = ''
for part in parts:
    s += part
"""
    stmt_fast = "s = ''.join(parts)"

    t_slow = min(timeit.repeat(stmt_slow, setup=setup, number=1000, repeat=5)) / 1000
    t_fast = min(timeit.repeat(stmt_fast, setup=setup, number=1000, repeat=5)) / 1000

    report("s += part", t_slow, "''.join(parts)", t_fast)


# ── 3. builtin sum vs manual loop ─────────────────────────────────────────

def bench_builtin_sum():
    header("3. builtin sum() vs manual loop")

    setup = "xs = list(range(100_000))"
    stmt_slow = """
total = 0
for x in xs:
    total += x
"""
    stmt_fast = "total = sum(xs)"

    t_slow = min(timeit.repeat(stmt_slow, setup=setup, number=100, repeat=5)) / 100
    t_fast = min(timeit.repeat(stmt_fast, setup=setup, number=100, repeat=5)) / 100

    report("manual loop", t_slow, "sum()", t_fast)


# ── 4. lru_cache memoization ──────────────────────────────────────────────

def bench_lru_cache():
    header("4. lru_cache memoization (fib(32))")

    setup_naive = """
def fib(n):
    if n < 2: return n
    return fib(n - 1) + fib(n - 2)
"""
    setup_cached = """
from functools import lru_cache

@lru_cache(maxsize=None)
def fib(n):
    if n < 2: return n
    return fib(n - 1) + fib(n - 2)
"""
    # naive: only 1 call per round (it's already slow)
    t_naive  = min(timeit.repeat("fib(32)", setup=setup_naive,  number=1,    repeat=3))
    # cached: many calls to amortize overhead; clear cache each round via fresh setup
    t_cached = min(timeit.repeat("fib(32)", setup=setup_cached, number=1000, repeat=5)) / 1000

    report("fib(32) naive", t_naive, "fib(32) @lru_cache", t_cached)


# ── 5. NumPy matmul vs Python loops ───────────────────────────────────────

def bench_numpy():
    header("5. NumPy matmul vs Python loops (200x200)")

    setup = """
import numpy as np
n = 200
A_np = np.random.rand(n, n)
B_np = np.random.rand(n, n)
A_py = A_np.tolist()
B_py = B_np.tolist()
"""
    stmt_slow = """
n = len(A_py)
C = [[0.0] * n for _ in range(n)]
for i in range(n):
    for k in range(n):
        for j in range(n):
            C[i][j] += A_py[i][k] * B_py[k][j]
"""
    stmt_fast = "C = A_np @ B_np"

    t_slow = min(timeit.repeat(stmt_slow, setup=setup, number=1,     repeat=3))
    t_fast = min(timeit.repeat(stmt_fast, setup=setup, number=1000,  repeat=5)) / 1000

    report("Python loops", t_slow, "NumPy A @ B", t_fast)


# ── 6. data containers ────────────────────────────────────────────────────

def bench_data_containers():
    header("6. data containers: creation cost (100k objects)")

    N = 100_000

    configs = [
        ("plain tuple", "", f"[(1.0, 2.0, 3.0) for _ in range({N})]"),
        ("plain class",
         """
class Point:
    def __init__(self, x, y, z):
        self.x = x; self.y = y; self.z = z
""",
         f"[Point(1.0, 2.0, 3.0) for _ in range({N})]"),
        ("dataclass",
         """
from dataclasses import dataclass
@dataclass
class Point:
    x: float; y: float; z: float
""",
         f"[Point(1.0, 2.0, 3.0) for _ in range({N})]"),
        ("dataclass(slots=True)",
         """
from dataclasses import dataclass
@dataclass(slots=True)
class Point:
    x: float; y: float; z: float
""",
         f"[Point(1.0, 2.0, 3.0) for _ in range({N})]"),
        ("NamedTuple",
         """
from collections import namedtuple
Point = namedtuple('Point', ['x', 'y', 'z'])
""",
         f"[Point(1.0, 2.0, 3.0) for _ in range({N})]"),
        ("Pydantic BaseModel",
         """
from pydantic import BaseModel
class Point(BaseModel):
    x: float; y: float; z: float
""",
         f"[Point(x=1.0, y=2.0, z=3.0) for _ in range({N})]"),
    ]

    results = []
    for name, setup, stmt in configs:
        t = min(timeit.repeat(stmt, setup=setup, number=5, repeat=3)) / 5
        results.append((name, t))

    baseline = results[0][1]
    print(f"  {'Container':28s} {'Time':>10s} {'Relative':>10s}")
    print(f"  {'-' * 28} {'-' * 10} {'-' * 10}")
    for name, t in results:
        rel = t / baseline
        print(f"  {name:28s} {t * 1000:8.1f} ms {rel:8.1f}x")

    header("6b. data containers: create + access (100k objects)")

    access_configs = [
        ("plain tuple", "",
         f"""
points = [(float(i), float(i+1), float(i+2)) for i in range({N})]
total = 0.0
for p in points:
    total += p[0] + p[1] + p[2]
"""),
        ("plain class",
         """
class Point:
    def __init__(self, x, y, z):
        self.x = x; self.y = y; self.z = z
""",
         f"""
points = [Point(float(i), float(i+1), float(i+2)) for i in range({N})]
total = 0.0
for p in points:
    total += p.x + p.y + p.z
"""),
        ("dataclass(slots=True)",
         """
from dataclasses import dataclass
@dataclass(slots=True)
class Point:
    x: float; y: float; z: float
""",
         f"""
points = [Point(float(i), float(i+1), float(i+2)) for i in range({N})]
total = 0.0
for p in points:
    total += p.x + p.y + p.z
"""),
        ("Pydantic BaseModel",
         """
from pydantic import BaseModel
class Point(BaseModel):
    x: float; y: float; z: float
""",
         f"""
points = [Point(x=float(i), y=float(i+1), z=float(i+2)) for i in range({N})]
total = 0.0
for p in points:
    total += p.x + p.y + p.z
"""),
    ]

    results2 = []
    for name, setup, stmt in access_configs:
        t = min(timeit.repeat(stmt, setup=setup, number=5, repeat=3)) / 5
        results2.append((name, t))

    baseline2 = results2[0][1]
    print(f"  {'Container':28s} {'Time':>10s} {'Relative':>10s}")
    print(f"  {'-' * 28} {'-' * 10} {'-' * 10}")
    for name, t in results2:
        rel = t / baseline2
        print(f"  {name:28s} {t * 1000:8.1f} ms {rel:8.1f}x")


# ── Main ──────────────────────────────────────────────────────────────────

def main():
    print(f"Python {sys.version}")
    try:
        import pydantic
        print(f"Pydantic {pydantic.__version__}")
    except ImportError:
        print("Pydantic not installed — skipping container benchmark")
    try:
        import numpy as np
        print(f"NumPy {np.__version__}")
    except ImportError:
        print("NumPy not installed — skipping matmul benchmark")

    bench_membership()
    bench_string_concat()
    bench_builtin_sum()
    bench_lru_cache()

    try:
        import numpy  # noqa: F401
        bench_numpy()
    except ImportError:
        print("\n  [skipped: numpy not installed]")

    try:
        import pydantic  # noqa: F401
        bench_data_containers()
    except ImportError:
        print("\n  [skipped: pydantic not installed]")

    print()


if __name__ == "__main__":
    main()
