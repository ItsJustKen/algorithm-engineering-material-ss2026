/**
 * graph.h — Two graph representations with identical APIs, different memory layouts.
 *
 * AdjacencyList: vector<vector<int>> — the familiar textbook representation.
 *   Each vertex owns a separate heap allocation for its neighbor list.
 *   Easy to build incrementally, but neighbor lists are scattered in memory.
 *
 * CSR (Compressed Sparse Row): two flat arrays — offsets[] and targets[].
 *   All neighbor lists packed contiguously. Cache-friendly sequential access,
 *   but the graph must be fully known at construction time (no incremental edits).
 *
 * Both expose the same interface so benchmarks can compare them fairly:
 *   - num_vertices(), num_edges()
 *   - degree(v)
 *   - neighbors(v) → a range usable in range-for loops
 *
 * Header-only so tests and benchmarks can both #include it directly.
 */

#pragma once

#include <algorithm>   // std::sort (CSR constructor)
#include <cstddef>
#include <span>
#include <vector>

// ============================================================
// AdjacencyList — vector<vector<int>>
// ============================================================

/**
 * Standard adjacency list: each vertex has its own vector of neighbors.
 *
 * Memory layout (simplified, 4 vertices):
 *
 *   adj_[0] → heap block A: [2, 3]
 *   adj_[1] → heap block B: [0]
 *   adj_[2] → heap block C: [0, 1, 3]
 *   adj_[3] → heap block D: [0, 2]
 *
 * Each inner vector is a separate heap allocation. Iterating neighbors of
 * one vertex is fast (sequential within that block), but iterating all
 * vertices' neighbors jumps between unrelated heap locations → cache misses.
 */
class AdjacencyList {
public:
    explicit AdjacencyList(int n) : adj_(n) {}

    void add_edge(int u, int v) {
        adj_[u].push_back(v);
        adj_[v].push_back(u);
    }

    [[nodiscard]] int num_vertices() const { return static_cast<int>(adj_.size()); }

    [[nodiscard]] std::size_t num_edges() const {
        std::size_t total = 0;
        for (const auto& neighbors : adj_) {
            total += neighbors.size();
        }
        return total / 2;  // each edge counted twice (undirected)
    }

    [[nodiscard]] int degree(int v) const {
        return static_cast<int>(adj_[v].size());
    }

    // Returns a view (span) over the neighbor list — no copy.
    [[nodiscard]] std::span<const int> neighbors(int v) const {
        return adj_[v];
    }

private:
    std::vector<std::vector<int>> adj_;
};

// ============================================================
// CSR (Compressed Sparse Row)
// ============================================================

/**
 * Compressed Sparse Row: all neighbors packed into one flat array.
 *
 * Memory layout (same 4-vertex graph):
 *
 *   offsets_: [0,  2,  3,  6,  8]
 *   targets_: [2, 3, | 0, | 0, 1, 3, | 0, 2]
 *                ^       ^    ^          ^
 *                v=0     v=1  v=2        v=3
 *
 * Neighbors of vertex v are targets_[offsets_[v] .. offsets_[v+1]].
 * Everything lives in two contiguous allocations. Sequential iteration
 * over all vertices' neighbors walks memory linearly → cache-friendly.
 *
 * Trade-off: the graph structure is immutable after construction.
 */
class CSR {
public:
    /**
     * Build CSR from an edge list. Sorts neighbors per vertex for
     * reproducibility (not required by the format).
     */
    CSR(int n, const std::vector<std::pair<int, int>>& edges)
        : offsets_(n + 1, 0) {
        // Count degrees.
        for (auto [u, v] : edges) {
            offsets_[u + 1]++;
            offsets_[v + 1]++;
        }
        // Prefix sum → offsets_[v] = start index for vertex v's neighbors.
        for (int i = 1; i <= n; i++) {
            offsets_[i] += offsets_[i - 1];
        }
        // Fill targets using a write cursor per vertex.
        targets_.resize(offsets_[n]);
        std::vector<int> cursor(offsets_.begin(), offsets_.end() - 1);
        for (auto [u, v] : edges) {
            targets_[cursor[u]++] = v;
            targets_[cursor[v]++] = u;
        }
        // Sort each vertex's neighbors for reproducible iteration order.
        for (int v = 0; v < n; v++) {
            std::sort(targets_.begin() + offsets_[v],
                      targets_.begin() + offsets_[v + 1]);
        }
    }

    [[nodiscard]] int num_vertices() const {
        return static_cast<int>(offsets_.size()) - 1;
    }

    [[nodiscard]] std::size_t num_edges() const {
        return targets_.size() / 2;
    }

    [[nodiscard]] int degree(int v) const {
        return offsets_[v + 1] - offsets_[v];
    }

    // Returns a view into the flat targets_ array — no copy, no indirection.
    [[nodiscard]] std::span<const int> neighbors(int v) const {
        return {targets_.data() + offsets_[v],
                static_cast<std::size_t>(offsets_[v + 1] - offsets_[v])};
    }

private:
    std::vector<int> offsets_;  // size = num_vertices + 1
    std::vector<int> targets_;  // size = 2 * num_edges (undirected)
};
