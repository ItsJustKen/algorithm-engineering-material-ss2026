"""
Basic correctness tests for the C++ TSP routines.

Run:
    pytest tests/
"""

import random

import fast_tsp


def _make_square_cities() -> list[tuple[float, float]]:
    """Four cities at corners of a unit square."""
    return [
        (0.0, 0.0),
        (1.0, 0.0),
        (1.0, 1.0),
        (0.0, 1.0),
    ]


def _make_random_cities(n: int, *, seed: int) -> list[tuple[float, float]]:
    rng = random.Random(seed)
    return [(rng.uniform(0, 1000), rng.uniform(0, 1000)) for _ in range(n)]


# ---- Free function tests (coordinate-pair API) ----

def test_nearest_neighbor_visits_all():
    cities = _make_square_cities()
    tour = fast_tsp.nearest_neighbor(cities)
    assert sorted(tour) == [0, 1, 2, 3], f"Tour doesn't visit all cities: {tour}"


def test_tour_length_square():
    cities = _make_square_cities()
    tour = [0, 1, 2, 3]  # perimeter of unit square
    length = fast_tsp.tour_length(cities, tour)
    assert abs(length - 4.0) < 1e-10, f"Expected 4.0, got {length}"


def test_two_opt_does_not_worsen():
    cities = _make_random_cities(100, seed=42)
    tour = fast_tsp.nearest_neighbor(cities)
    length_before = fast_tsp.tour_length(cities, tour)

    improved_tour = fast_tsp.two_opt_improve(cities, tour)
    length_after = fast_tsp.tour_length(cities, improved_tour)

    assert length_after <= length_before + 1e-6, (
        f"2-opt worsened tour: {length_before:.2f} -> {length_after:.2f}"
    )


def test_two_opt_returns_valid_tour():
    n = 50
    cities = _make_random_cities(n, seed=123)
    tour = fast_tsp.nearest_neighbor(cities)
    improved = fast_tsp.two_opt_improve(cities, tour)
    assert sorted(improved) == list(range(n)), "Tour is not a valid permutation"


# ---- DistanceMatrix tests ----

def test_distance_matrix_size():
    cities = _make_square_cities()
    dm = fast_tsp.DistanceMatrix(cities)
    assert dm.size == 4


def test_distance_matrix_query():
    cities = _make_square_cities()
    dm = fast_tsp.DistanceMatrix(cities)
    # Adjacent corners of unit square → distance 1.0
    assert abs(dm.query(0, 1) - 1.0) < 1e-10
    # Diagonal → distance sqrt(2)
    assert abs(dm.query(0, 2) - 2**0.5) < 1e-10
    # Symmetric
    assert dm.query(0, 1) == dm.query(1, 0)
    # Self-distance is zero
    assert dm.query(0, 0) == 0.0


def test_distance_matrix_nearest_neighbor():
    """DistanceMatrix overload produces the same tour as the coordinate-pair overload."""
    cities = _make_random_cities(200, seed=42)
    dm = fast_tsp.DistanceMatrix(cities)

    tour_np = fast_tsp.nearest_neighbor(cities)
    tour_dm = fast_tsp.nearest_neighbor(dm)
    assert tour_np == tour_dm


def test_distance_matrix_two_opt():
    """DistanceMatrix 2-opt produces the same result as the coordinate-pair version."""
    cities = _make_random_cities(100, seed=42)
    dm = fast_tsp.DistanceMatrix(cities)

    tour = fast_tsp.nearest_neighbor(cities)
    improved_np = fast_tsp.two_opt_improve(cities, tour)
    improved_dm = fast_tsp.two_opt_improve(dm, tour)

    # Same tour (both use identical algorithm, same floating-point values)
    assert improved_np == improved_dm


def test_distance_matrix_tour_length():
    """DistanceMatrix tour_length matches the coordinate-pair version."""
    cities = _make_square_cities()
    dm = fast_tsp.DistanceMatrix(cities)
    tour = [0, 1, 2, 3]
    length_np = fast_tsp.tour_length(cities, tour)
    length_dm = fast_tsp.tour_length(dm, tour)
    assert abs(length_np - length_dm) < 1e-10
