/**
 * graph_gen.h — Deterministic random graph generators for tests and benchmarks.
 *
 * Provides functions to create both AdjacencyList and CSR representations
 * of the same random graph (same seed → same edges → comparable benchmarks).
 *
 * Uses Erdős–Rényi G(n, m) model: n vertices, m edges chosen uniformly
 * at random (no self-loops, no parallel edges).
 */

#pragma once

#include <cassert>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include "graph.h"

/**
 * Generate m random edges on n vertices.
 * Fixed seed ensures reproducibility across runs.
 */
inline std::vector<std::pair<int, int>> random_edges(int n, std::size_t m,
                                                     unsigned seed = 42) {
    assert(m <= static_cast<std::size_t>(n) * (n - 1) / 2
           && "requested more edges than the complete graph has");
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, n - 1);

    // Use a set to reject duplicates and self-loops.
    std::set<std::pair<int, int>> edge_set;
    while (edge_set.size() < m) {
        int u = dist(rng);
        int v = dist(rng);
        if (u != v) {
            auto e = std::minmax(u, v);  // canonical order
            edge_set.insert(e);
        }
    }
    return {edge_set.begin(), edge_set.end()};
}

inline AdjacencyList make_adjlist(int n, const std::vector<std::pair<int, int>>& edges) {
    AdjacencyList g(n);
    for (auto [u, v] : edges) {
        g.add_edge(u, v);
    }
    return g;
}

inline CSR make_csr(int n, const std::vector<std::pair<int, int>>& edges) {
    return CSR(n, edges);
}
