import math
import time


def _dist(points, a, b):
    dx = points[a][0] - points[b][0]
    dy = points[a][1] - points[b][1]
    return math.sqrt(dx * dx + dy * dy)


def best_improvement_two_opt(
    points: list[tuple[float, float]],
    initial_tour: list[int],
    timeout: float = 10.0,
) -> list[int]:

    tour = initial_tour.copy()
    n = len(tour)

    deadline = time.perf_counter() + timeout

    while True:
        best_delta = 0
        best_i = -1
        best_j = -1

        for i in range(n - 1):
            for j in range(i + 2, n):

                # cyclic adjacency skip
                if i == 0 and j == n - 1:
                    continue

                if time.perf_counter() > deadline:
                    return tour

                a, b = tour[i], tour[i + 1]
                c, d = tour[j], tour[(j + 1) % n]

                current = _dist(points, a, b) + _dist(points, c, d)
                new = _dist(points, a, c) + _dist(points, b, d)

                delta = new - current

                if delta < best_delta:
                    best_delta = delta
                    best_i = i
                    best_j = j

        # no improvement found
        if best_i == -1:
            break

        # apply best move
        tour[best_i + 1:best_j + 1] = reversed(tour[best_i + 1:best_j + 1])

        if time.perf_counter() > deadline:
            break

    return tour
