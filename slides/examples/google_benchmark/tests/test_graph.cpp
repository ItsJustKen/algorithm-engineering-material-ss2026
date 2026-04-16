/**
 * Unit tests for graph.h — verify correctness before benchmarking.
 *
 * Tests both representations against the same graph to ensure they
 * agree on structure (degrees, neighbors, edge count). A fast wrong
 * answer is useless, so always test before you benchmark.
 */

#include <set>

#include <gtest/gtest.h>

#include <graph/graph.h>
#include <graph/graph_gen.h>

// --- Small hand-built graph for exact checks ---
// Triangle: 0-1, 1-2, 0-2

static std::vector<std::pair<int, int>> triangle_edges() {
    return {{0, 1}, {1, 2}, {0, 2}};
}

TEST(AdjacencyList, TriangleDegrees) {
    auto g = make_adjlist(3, triangle_edges());
    EXPECT_EQ(g.num_vertices(), 3);
    EXPECT_EQ(g.num_edges(), 3);
    EXPECT_EQ(g.degree(0), 2);
    EXPECT_EQ(g.degree(1), 2);
    EXPECT_EQ(g.degree(2), 2);
}

TEST(CSR, TriangleDegrees) {
    auto g = make_csr(3, triangle_edges());
    EXPECT_EQ(g.num_vertices(), 3);
    EXPECT_EQ(g.num_edges(), 3);
    EXPECT_EQ(g.degree(0), 2);
    EXPECT_EQ(g.degree(1), 2);
    EXPECT_EQ(g.degree(2), 2);
}

TEST(AdjacencyList, TriangleNeighbors) {
    auto g = make_adjlist(3, triangle_edges());
    // Vertex 0 should have neighbors {1, 2} (order may vary).
    std::set<int> nbrs(g.neighbors(0).begin(), g.neighbors(0).end());
    EXPECT_EQ(nbrs, (std::set<int>{1, 2}));
}

TEST(CSR, TriangleNeighbors) {
    auto g = make_csr(3, triangle_edges());
    // CSR sorts neighbors, so we can check exact order.
    auto nbrs = g.neighbors(0);
    ASSERT_EQ(nbrs.size(), 2);
    EXPECT_EQ(nbrs[0], 1);
    EXPECT_EQ(nbrs[1], 2);
}

// --- Both representations agree on a random graph ---

TEST(CrossCheck, DegreesMatch) {
    int n = 500;
    auto edges = random_edges(n, 2500);
    auto adj = make_adjlist(n, edges);
    auto csr = make_csr(n, edges);

    EXPECT_EQ(adj.num_vertices(), csr.num_vertices());
    EXPECT_EQ(adj.num_edges(), csr.num_edges());

    for (int v = 0; v < n; v++) {
        EXPECT_EQ(adj.degree(v), csr.degree(v)) << "degree mismatch at vertex " << v;
    }
}

TEST(CrossCheck, NeighborSetsMatch) {
    int n = 200;
    auto edges = random_edges(n, 1000);
    auto adj = make_adjlist(n, edges);
    auto csr = make_csr(n, edges);

    for (int v = 0; v < n; v++) {
        std::set<int> adj_nbrs(adj.neighbors(v).begin(), adj.neighbors(v).end());
        std::set<int> csr_nbrs(csr.neighbors(v).begin(), csr.neighbors(v).end());
        EXPECT_EQ(adj_nbrs, csr_nbrs) << "neighbor set mismatch at vertex " << v;
    }
}
