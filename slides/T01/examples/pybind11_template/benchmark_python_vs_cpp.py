"""
Example: Compare pure Python vs. pybind11 C++ for TSP 2-opt.

What this demonstrates:
- The speedup from moving an entire hot loop to C++
- A simple copy-once Python-list -> C++-vector boundary
- That tour quality is identical (same algorithm)

Prerequisites:
    pip install -e .   (builds the C++ extension; pybind11 is a build dependency)

Run:
    python benchmark_python_vs_cpp.py
"""

import math
import random
import time

# ---- Pure Python implementation ----

def py_distance(p1, p2):
    dx = p1[0] - p2[0]
    dy = p1[1] - p2[1]
    return math.sqrt(dx * dx + dy * dy)


def py_nearest_neighbor(cities):
    n = len(cities)
    visited = [False] * n
    tour = [0]
    visited[0] = True
    for _ in range(n - 1):
        current = tour[-1]
        best, best_d = -1, float("inf")
        for j in range(n):
            if not visited[j]:
                d = py_distance(cities[current], cities[j])
                if d < best_d:
                    best_d, best = d, j
        tour.append(best)
        visited[best] = True
    return tour


def py_two_opt(cities, tour):
    n = len(tour)
    improved = True
    while improved:
        improved = False
        for i in range(n - 1):
            for j in range(i + 2, n):
                a, b = cities[tour[i]], cities[tour[i + 1]]
                c, d = cities[tour[j]], cities[tour[(j + 1) % n]]
                if (py_distance(a, c) + py_distance(b, d)
                        < py_distance(a, b) + py_distance(c, d) - 1e-10):
                    tour[i + 1:j + 1] = tour[i + 1:j + 1][::-1]
                    improved = True
    return tour


def py_tour_length(cities, tour):
    return sum(py_distance(cities[tour[i]], cities[tour[(i + 1) % len(tour)]])
               for i in range(len(tour)))


if __name__ == "__main__":
    n_cities = 1000
    rng = random.Random(42)
    cities_list = [(rng.uniform(0, 1000), rng.uniform(0, 1000)) for _ in range(n_cities)]

    print(f"TSP benchmark, N = {n_cities}\n")

    # ---- Python version ----
    print("Python:")
    t0 = time.perf_counter()
    py_tour = py_nearest_neighbor(cities_list)
    t1 = time.perf_counter()
    print(f"  Nearest neighbor: {t1 - t0:.3f}s")

    py_tour = py_two_opt(cities_list, py_tour)
    t2 = time.perf_counter()
    print(f"  2-opt:            {t2 - t1:.3f}s")
    py_length = py_tour_length(cities_list, py_tour)
    print(f"  Tour length:      {py_length:.1f}")
    print(f"  Total:            {t2 - t0:.3f}s")

    # ---- C++ version (computing distances on the fly) ----
    try:
        import fast_tsp

        print("\nC++ (on-the-fly distances):")
        t0 = time.perf_counter()
        cpp_tour = fast_tsp.nearest_neighbor(cities_list)
        t1 = time.perf_counter()
        print(f"  Nearest neighbor: {t1 - t0:.3f}s")

        cpp_tour = fast_tsp.two_opt_improve(cities_list, cpp_tour)
        t2 = time.perf_counter()
        print(f"  2-opt:            {t2 - t1:.3f}s")
        cpp_length = fast_tsp.tour_length(cities_list, cpp_tour)
        print(f"  Tour length:      {cpp_length:.1f}")
        print(f"  Total:            {t2 - t0:.3f}s")

        # ---- C++ version (precomputed distance matrix) ----
        print("\nC++ (precomputed DistanceMatrix):")
        t0 = time.perf_counter()
        dm = fast_tsp.DistanceMatrix(cities_list)
        t1 = time.perf_counter()
        print(f"  Build matrix:     {t1 - t0:.3f}s")

        dm_tour = fast_tsp.nearest_neighbor(dm)
        t2 = time.perf_counter()
        print(f"  Nearest neighbor: {t2 - t1:.3f}s")

        dm_tour = fast_tsp.two_opt_improve(dm, dm_tour)
        t3 = time.perf_counter()
        print(f"  2-opt:            {t3 - t2:.3f}s")
        dm_length = fast_tsp.tour_length(dm, dm_tour)
        print(f"  Tour length:      {dm_length:.1f}")
        print(f"  Total:            {t3 - t0:.3f}s")

        print(f"\nTour quality: {'identical' if abs(py_length - cpp_length) < 1.0 else f'differs (Python={py_length:.1f}, C++={cpp_length:.1f})'}")

    except ImportError:
        print("\nC++ module not built. Run: pip install -e .")
        print("Then re-run this script.")
