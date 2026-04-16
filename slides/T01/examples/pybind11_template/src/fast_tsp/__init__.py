"""
fast_tsp — Example pybind11 package for Algorithm Engineering.

Demonstrates moving hot Python loops to C++ for ~50× speedup
on TSP 2-opt local search, using a simple Python-list to C++-vector boundary.

Public API (free functions accepting coordinate pairs or DistanceMatrix):
    fast_tsp.nearest_neighbor(cities)  -> list[int]
    fast_tsp.two_opt_improve(cities, tour) -> list[int]
    fast_tsp.tour_length(cities, tour) -> float
    fast_tsp.DistanceMatrix(cities) -> DistanceMatrix

All functions accept `cities` as a sequence of `(x, y)` pairs or a DistanceMatrix.
"""

# Must match pyproject.toml [project].version and _bindings.cpp FAST_TSP_VERSION.
# tests/test_version.py enforces consistency across all three.
__version__ = "0.1.0"

# Re-export the C++ functions so users write `fast_tsp.nearest_neighbor(...)`,
# not `fast_tsp._native.nearest_neighbor(...)`.
from fast_tsp._native import (
    DistanceMatrix,
    nearest_neighbor,
    tour_length,
    two_opt_improve,
)

# Expose the C++ extension's version for comparison in tests.
# If this doesn't match __version__, the extension is stale.
from fast_tsp._native import __version__ as __native_version__

__all__ = [
    "__version__",
    "__native_version__",
    "DistanceMatrix",
    "nearest_neighbor",
    "tour_length",
    "two_opt_improve",
]
