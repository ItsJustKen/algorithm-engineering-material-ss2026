/**
 * tsp.h — Pure C++ declarations for TSP routines.
 *
 * This header has no pybind11 dependency. The algorithms work on
 * Point collections and std::vector, making them
 * testable and usable from plain C++ without any Python involvement.
 *
 * Keeping implementation separate from bindings means:
 * - The algorithms can be unit-tested in C++ directly.
 * - Changing the Python interface doesn't require recompiling the algorithms.
 * - The binding file (_bindings.cpp) stays short and reviewable.
 */

#pragma once

#include <cmath>
#include <span>
#include <vector>

/**
 * Simple, explicit coordinate type.
 *
 * Using a named Point type keeps the algorithm code readable and avoids
 * teaching numpy buffer semantics in the binding layer. The Python binding
 * can copy a list of coordinate pairs into std::vector<Point> once, then all
 * O(N²) work happens on the C++ side.
 */
struct Point {
    double x;
    double y;
};

/**
 * Euclidean distance between cities i and j.
 *
 * Inline because this is called O(N²) times per 2-opt pass —
 * function call overhead would be significant at that frequency.
 */
inline double dist(std::span<const Point> cities, int i, int j) {
    double dx = cities[i].x - cities[j].x;
    double dy = cities[i].y - cities[j].y;
    // Not using std::hypot: it handles overflow/underflow edge cases we
    // don't need, and is measurably slower in tight loops.
    return std::sqrt(dx * dx + dy * dy);
}

// ---- Free functions (compute distances on the fly from coordinates) ----

std::vector<int> two_opt_improve(std::span<const Point> cities, std::vector<int> tour);
std::vector<int> nearest_neighbor(std::span<const Point> cities);
double tour_length(std::span<const Point> cities, const std::vector<int>& tour);

// ---- Free functions (using precomputed DistanceMatrix) ----
// Forward-declared; DistanceMatrix is defined below.

class DistanceMatrix;

std::vector<int> two_opt_improve(const DistanceMatrix& dm, std::vector<int> tour);
std::vector<int> nearest_neighbor(const DistanceMatrix& dm);
double tour_length(const DistanceMatrix& dm, const std::vector<int>& tour);

// ---- DistanceMatrix (precomputed pairwise distances) ----

/**
 * Precomputed N×N distance matrix for TSP.
 *
 * Computing sqrt is expensive in tight loops. For 2-opt, which checks O(N²)
 * pairs per pass over multiple passes, precomputing all distances into a flat
 * array trades O(N²) memory for a significant speedup: each distance lookup
 * becomes a single array read instead of two subtractions, a multiply, and a sqrt.
 *
 * Memory: N=1000 → 1M doubles → 8 MB. Reasonable up to ~10k cities.
 */
class DistanceMatrix {
public:
    /**
     * Build the full N×N distance matrix from city coordinates.
     * Construction is O(N²) — the cost is paid once upfront.
     */
    explicit DistanceMatrix(std::span<const Point> cities);

    int size() const { return n_; }

    /** Distance between cities i and j. O(1) lookup. */
    double operator()(int i, int j) const { return data_[i * n_ + j]; }

private:
    int n_;
    std::vector<double> data_;  // row-major N×N
};
