/**
 * tsp.cpp — Pure C++ implementation of TSP routines.
 *
 * No pybind11 dependency. This file compiles as plain C++20.
 */

#include "tsp.h"

#include <algorithm>   // std::reverse
#include <limits>      // std::numeric_limits

// ============================================================
// Free functions (compute distances on the fly from coordinates)
// ============================================================

std::vector<int> two_opt_improve(std::span<const Point> cities, std::vector<int> tour) {
    int n = static_cast<int>(tour.size());

    bool improved = true;
    while (improved) {
        improved = false;
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 2; j < n; j++) {
                int a = tour[i], b = tour[i + 1];
                int c = tour[j], d = tour[(j + 1) % n];

                double old_d = dist(cities[a], cities[b]) + dist(cities[c], cities[d]);
                double new_d = dist(cities[a], cities[c]) + dist(cities[b], cities[d]);

                // 1e-10 tolerance avoids infinite loops from floating-point noise.
                if (new_d < old_d - 1e-10) {
                    std::reverse(tour.begin() + i + 1, tour.begin() + j + 1);
                    improved = true;
                }
            }
        }
    }
    return tour;
}

std::vector<int> nearest_neighbor(std::span<const Point> cities) {
    int n = static_cast<int>(cities.size());
    std::vector<bool> visited(n, false);
    std::vector<int> tour;
    tour.reserve(n);
    tour.push_back(0);
    visited[0] = true;

    for (int step = 1; step < n; step++) {
        int current = tour.back();
        int best = -1;
        double best_d = std::numeric_limits<double>::infinity();
        for (int j = 0; j < n; j++) {
            if (!visited[j]) {
                double d = dist(cities[current], cities[j]);
                if (d < best_d) {
                    best_d = d;
                    best = j;
                }
            }
        }
        tour.push_back(best);
        visited[best] = true;
    }
    return tour;
}

double tour_length(std::span<const Point> cities, const std::vector<int>& tour) {
    int n = static_cast<int>(tour.size());
    double total = 0.0;
    for (int i = 0; i < n; i++) {
        total += dist(cities[tour[i]], cities[tour[(i + 1) % n]]);
    }
    return total;
}

// ============================================================
// DistanceMatrix
// ============================================================

DistanceMatrix::DistanceMatrix(std::span<const Point> cities)
    : n_(static_cast<int>(cities.size())), data_(static_cast<size_t>(n_) * n_) {
    for (int i = 0; i < n_; i++) {
        data_[i * n_ + i] = 0.0;
        for (int j = i + 1; j < n_; j++) {
            double d = dist(cities[i], cities[j]);
            data_[i * n_ + j] = d;
            data_[j * n_ + i] = d;
        }
    }
}

// ============================================================
// Free functions (using precomputed DistanceMatrix)
// ============================================================

std::vector<int> two_opt_improve(const DistanceMatrix& dm, std::vector<int> tour) {
    int n = static_cast<int>(tour.size());

    bool improved = true;
    while (improved) {
        improved = false;
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 2; j < n; j++) {
                int a = tour[i], b = tour[i + 1];
                int c = tour[j], d = tour[(j + 1) % n];

                double old_d = dm(a, b) + dm(c, d);
                double new_d = dm(a, c) + dm(b, d);

                if (new_d < old_d - 1e-10) {
                    std::reverse(tour.begin() + i + 1, tour.begin() + j + 1);
                    improved = true;
                }
            }
        }
    }
    return tour;
}

std::vector<int> nearest_neighbor(const DistanceMatrix& dm) {
    int n = dm.size();
    std::vector<bool> visited(n, false);
    std::vector<int> tour;
    tour.reserve(n);
    tour.push_back(0);
    visited[0] = true;

    for (int step = 1; step < n; step++) {
        int current = tour.back();
        int best = -1;
        double best_d = std::numeric_limits<double>::infinity();
        for (int j = 0; j < n; j++) {
            if (!visited[j]) {
                double d = dm(current, j);
                if (d < best_d) {
                    best_d = d;
                    best = j;
                }
            }
        }
        tour.push_back(best);
        visited[best] = true;
    }
    return tour;
}

double tour_length(const DistanceMatrix& dm, const std::vector<int>& tour) {
    int n = static_cast<int>(tour.size());
    double total = 0.0;
    for (int i = 0; i < n; i++) {
        total += dm(tour[i], tour[(i + 1) % n]);
    }
    return total;
}
