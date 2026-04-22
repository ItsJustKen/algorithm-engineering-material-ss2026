#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace py = pybind11;

using Point = std::pair<double, double>;
using Clock = std::chrono::steady_clock;

// -----------------------------
// Distance
// -----------------------------
static inline double dist(const std::vector<Point> &points, int a, int b) {
  double dx = points[a].first - points[b].first;
  double dy = points[a].second - points[b].second;
  return std::sqrt(dx * dx + dy * dy);
}

// -----------------------------
// Tour length
// -----------------------------
static double tour_length(const std::vector<Point> &points,
                          const std::vector<int> &tour) {
  double total = 0.0;
  int n = tour.size();

  for (int i = 0; i < n; ++i) {
    total += dist(points, tour[i], tour[(i + 1) % n]);
  }
  return total;
}

// -----------------------------
// Random tour
// -----------------------------
static std::vector<int> make_random_tour(int n, int seed) {
  std::vector<int> tour(n);
  std::iota(tour.begin(), tour.end(), 0);

  std::mt19937 rng(seed);
  std::shuffle(tour.begin(), tour.end(), rng);

  return tour;
}

// =======================================================
// 1. FIRST IMPROVEMENT
// =======================================================
static std::vector<int> two_opt_first_improvement(
    const std::vector<Point> &points, std::vector<int> tour,
    double timeout_seconds) {

  int n = tour.size();
  auto deadline = Clock::now() + std::chrono::duration<double>(timeout_seconds);

  while (true) {
    bool improved = false;

    for (int i = 0; i < n - 1; ++i) {
      for (int j = i + 2; j < n; ++j) {

        if (i == 0 && j == n - 1) continue;

        double delta =
            dist(points, tour[i], tour[j]) +
            dist(points, tour[i + 1], tour[(j + 1) % n]) -
            dist(points, tour[i], tour[i + 1]) -
            dist(points, tour[j], tour[(j + 1) % n]);

        if (delta < 0) {
          std::reverse(tour.begin() + i + 1, tour.begin() + j + 1);
          improved = true;
          break;
        }

        if (Clock::now() >= deadline) return tour;
      }

      if (improved) break;
    }

    if (Clock::now() >= deadline) return tour;
    if (!improved) break;
  }

  return tour;
}

// =======================================================
// 2. FULL SCAN
// =======================================================
static std::vector<int> two_opt_full_scan(
    const std::vector<Point> &points, std::vector<int> tour,
    double timeout_seconds) {

  int n = tour.size();
  auto deadline = Clock::now() + std::chrono::duration<double>(timeout_seconds);

  while (true) {
    bool improved = false;

    for (int i = 0; i < n - 1; ++i) {
      for (int j = i + 2; j < n; ++j) {

        if (i == 0 && j == n - 1) continue;

        double delta =
            dist(points, tour[i], tour[j]) +
            dist(points, tour[i + 1], tour[(j + 1) % n]) -
            dist(points, tour[i], tour[i + 1]) -
            dist(points, tour[j], tour[(j + 1) % n]);

        if (delta < 0) {
          std::reverse(tour.begin() + i + 1, tour.begin() + j + 1);
          improved = true;
        }

        if (Clock::now() >= deadline) return tour;
      }
    }

    if (!improved) break;
    if (Clock::now() >= deadline) return tour;
  }

  return tour;
}

// =======================================================
// 3. BEST IMPROVEMENT
// =======================================================
static std::vector<int> two_opt_best_improvement(
    const std::vector<Point> &points, std::vector<int> tour,
    double timeout_seconds) {

  int n = tour.size();
  auto deadline = Clock::now() + std::chrono::duration<double>(timeout_seconds);

  while (true) {
    double best_delta = 0.0;
    int best_i = -1, best_j = -1;

    for (int i = 0; i < n - 1; ++i) {
      for (int j = i + 2; j < n; ++j) {

        if (i == 0 && j == n - 1) continue;

        double delta =
            dist(points, tour[i], tour[j]) +
            dist(points, tour[i + 1], tour[(j + 1) % n]) -
            dist(points, tour[i], tour[i + 1]) -
            dist(points, tour[j], tour[(j + 1) % n]);

        if (delta < best_delta) {
          best_delta = delta;
          best_i = i;
          best_j = j;
        }

        if (Clock::now() >= deadline) return tour;
      }
    }

    if (best_i < 0) break;

    std::reverse(tour.begin() + best_i + 1, tour.begin() + best_j + 1);

    if (Clock::now() >= deadline) return tour;
  }

  return tour;
}

// =======================================================
// 4. PARALLEL
// =======================================================
std::vector<int> parallel_two_opt(const std::vector<Point> &points,
                                  int num_threads,
                                  int base_seed,
                                  double timeout_seconds) {

  std::vector<std::thread> threads;
  std::vector<std::vector<int>> results(num_threads);
  std::vector<double> lengths(num_threads);

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back([&, t]() {
      auto tour = make_random_tour(points.size(), base_seed + t);
      tour = two_opt_full_scan(points, tour, timeout_seconds);
      results[t] = tour;
      lengths[t] = tour_length(points, tour);
    });
  }

  for (auto &th : threads) th.join();

  int best = 0;
  for (int i = 1; i < num_threads; ++i) {
    if (lengths[i] < lengths[best]) best = i;
  }

  return results[best];
}

// =======================================================
// PYBIND11 BINDINGS (DO NOT CHANGE)
// =======================================================
PYBIND11_MODULE(_core, m) {
  m.def("cpp_first_improvement", &two_opt_first_improvement,
        py::arg("points"), py::arg("initial_tour"),
        py::arg("timeout") = 10.0);

  m.def("cpp_full_scan", &two_opt_full_scan,
        py::arg("points"), py::arg("initial_tour"),
        py::arg("timeout") = 10.0);

  m.def("cpp_best_improvement", &two_opt_best_improvement,
        py::arg("points"), py::arg("initial_tour"),
        py::arg("timeout") = 10.0);

  m.def("parallel_two_opt", &parallel_two_opt,
        py::arg("points"),
        py::arg("num_threads") = 4,
        py::arg("base_seed") = 0,
        py::arg("timeout") = 10.0);
}
