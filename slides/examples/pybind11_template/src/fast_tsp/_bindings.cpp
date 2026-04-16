/**
 * _bindings.cpp — pybind11 module definition for fast_tsp._native.
 *
 * This file is the only place that depends on pybind11. It wraps the pure
 * C++ functions from tsp.h with Python-facing signatures: converting
 * Python coordinate pairs into std::vector<Point> and Python lists ↔ std::vector.
 *
 * Keeping bindings separate from implementation means:
 * - tsp.h/tsp.cpp can be tested and used from plain C++.
 * - Changing the Python interface doesn't recompile the algorithms.
 * - This file stays short and easy to review.
 */

// pybind11 core: Python/C++ type conversion, module definition macros.
#include <pybind11/pybind11.h>
// Automatic conversion between Python lists and std::vector.
#include <pybind11/stl.h>

#include <array>

#include "tsp.h"

namespace py = pybind11;

// Must match pyproject.toml [project].version and __init__.py __version__.
// tests/test_version.py checks all three agree. A mismatch after
// `pip install -e .` means the C++ wasn't recompiled.
#define FAST_TSP_VERSION "0.1.0"

// Module name "_native" matches the import in __init__.py:
//   from fast_tsp._native import nearest_neighbor, ...
// The leading underscore signals "private implementation detail".
PYBIND11_MODULE(_native, m) {
    m.doc() = "fast_tsp C++ extension — TSP routines via pybind11";

    // Expose version so Python tests can detect stale builds.
    m.attr("__version__") = FAST_TSP_VERSION;

    // ---- Free functions (compute distances on the fly) ----
    // Each wrapper copies Python coordinate pairs into std::vector<Point>
    // once, then delegates to the pure C++ function. For the O(N²) algorithms
    // here, this O(N) copy is negligible compared with the saved interpreter overhead.

    // GIL is released before long-running pure C++ computations so that
    // other Python threads are not blocked. This is safe because the lambdas
    // extract all data from Python objects *before* releasing the GIL and
    // never touch Python objects during the C++ computation.

    m.def("two_opt_improve",
          [](const std::vector<std::array<double, 2>>& cities, std::vector<int> tour) {
              std::vector<Point> points; points.reserve(cities.size());
              for (auto& c : cities) points.push_back({c[0], c[1]});
              py::gil_scoped_release release;
              return two_opt_improve(points, std::move(tour));
          },
          "Improve a TSP tour using 2-opt local search",
          py::arg("cities"), py::arg("tour"));

    m.def("nearest_neighbor",
          [](const std::vector<std::array<double, 2>>& cities) {
              std::vector<Point> points; points.reserve(cities.size());
              for (auto& c : cities) points.push_back({c[0], c[1]});
              py::gil_scoped_release release;
              return nearest_neighbor(points);
          },
          "Construct a tour using nearest neighbor heuristic",
          py::arg("cities"));

    m.def("tour_length",
          [](const std::vector<std::array<double, 2>>& cities, const std::vector<int>& tour) {
              std::vector<Point> points; points.reserve(cities.size());
              for (auto& c : cities) points.push_back({c[0], c[1]});
              return tour_length(points, tour);
          },
          "Compute total tour length",
          py::arg("cities"), py::arg("tour"));

    // ---- DistanceMatrix class ----
    // Precomputes all pairwise distances once. Algorithms using it replace
    // sqrt calls with flat-array lookups, which is significantly faster
    // for repeated access (e.g., multi-pass 2-opt).

    py::class_<DistanceMatrix>(m, "DistanceMatrix")
        .def(py::init([](const std::vector<std::array<double, 2>>& cities) {
                 std::vector<Point> points; points.reserve(cities.size());
                 for (auto& c : cities) points.push_back({c[0], c[1]});
                 return DistanceMatrix(points);
             }),
             "Build N×N distance matrix from a sequence of coordinate pairs",
             py::arg("cities"))
        .def_property_readonly("size", &DistanceMatrix::size,
             "Number of cities")
        .def("query", &DistanceMatrix::operator(),
             "Distance between cities i and j (O(1) lookup)",
             py::arg("i"), py::arg("j"));

    // ---- Free functions taking DistanceMatrix ----
    // Overloads of the same algorithms, but using precomputed distances.
    // Python distinguishes them by argument type (coordinate list vs DistanceMatrix).

    m.def("two_opt_improve",
          [](const DistanceMatrix& dm, std::vector<int> tour) {
              py::gil_scoped_release release;
              return two_opt_improve(dm, std::move(tour));
          },
          "Improve a TSP tour using 2-opt (with precomputed distances)",
          py::arg("distances"), py::arg("tour"));

    m.def("nearest_neighbor",
          [](const DistanceMatrix& dm) {
              py::gil_scoped_release release;
              return nearest_neighbor(dm);
          },
          "Construct a nearest neighbor tour (with precomputed distances)",
          py::arg("distances"));

    m.def("tour_length",
          [](const DistanceMatrix& dm, const std::vector<int>& tour) {
              return tour_length(dm, tour);
          },
          "Compute total tour length (with precomputed distances)",
          py::arg("distances"), py::arg("tour"));
}
