/**
 * Microbenchmarks: AdjacencyList vs. CSR on typical graph operations.
 *
 * What this demonstrates:
 * - Same algorithm, same graph, different data layout → different performance
 * - CSR's contiguous memory wins on sequential scans (cache-line prefetching)
 * - AdjacencyList's scattered heap allocations cause cache misses
 * - The gap grows with graph size (exceeding cache capacity)
 *
 * Build & run:
 *     cmake -B build -DCMAKE_BUILD_TYPE=Release
 *     cmake --build build
 *     ./build/benchmarks/bm_graph
 *
 * Useful flags:
 *     --benchmark_filter=BFS          Run only BFS benchmarks
 *     --benchmark_repetitions=5       Statistical stability
 *     --benchmark_format=json         Machine-readable output
 */

#include <queue>

#include <benchmark/benchmark.h>

#include <graph/graph.h>
#include <graph/graph_gen.h>

// --- Graph parameters ---
// Average degree 10 is typical for sparse graphs (road networks, social graphs).
// The benchmark varies vertex count; edge count scales as 5 * n (undirected → degree ~10).
static constexpr int AVG_DEGREE = 10;

static std::size_t num_edges(int n) {
    return static_cast<std::size_t>(n) * AVG_DEGREE / 2;
}

// ============================================================
// Benchmark 1: Iterate all neighbors of all vertices
// ============================================================
// This is the core loop of most graph algorithms (Dijkstra, BFS, PageRank, etc.).
// Measures raw memory access throughput: CSR walks memory linearly, AdjacencyList
// chases pointers to scattered heap blocks.

static void BM_iterate_neighbors_adjlist(benchmark::State& state) {
    int n = static_cast<int>(state.range(0));
    auto edges = random_edges(n, num_edges(n));
    auto graph = make_adjlist(n, edges);

    for (auto _ : state) {
        long sum = 0;
        for (int v = 0; v < graph.num_vertices(); v++) {
            for (int u : graph.neighbors(v)) {
                sum += u;  // minimal work per neighbor — isolates memory access cost
            }
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(graph.num_edges() * 2));
}

static void BM_iterate_neighbors_csr(benchmark::State& state) {
    int n = static_cast<int>(state.range(0));
    auto edges = random_edges(n, num_edges(n));
    auto graph = make_csr(n, edges);

    for (auto _ : state) {
        long sum = 0;
        for (int v = 0; v < graph.num_vertices(); v++) {
            for (int u : graph.neighbors(v)) {
                sum += u;
            }
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(graph.num_edges() * 2));
}

// ============================================================
// Benchmark 2: BFS traversal from vertex 0
// ============================================================
// A real algorithm. BFS accesses neighbors in vertex-discovery order, which is
// essentially random with respect to memory layout. CSR still wins because each
// neighbors(v) call reads a contiguous slice, even if the vertices are visited
// out of order.

template <typename Graph>
static int bfs(const Graph& graph, int start) {
    int n = graph.num_vertices();
    std::vector<bool> visited(n, false);
    std::queue<int> queue;
    queue.push(start);
    visited[start] = true;
    int count = 0;

    while (!queue.empty()) {
        int v = queue.front();
        queue.pop();
        count++;
        for (int u : graph.neighbors(v)) {
            if (!visited[u]) {
                visited[u] = true;
                queue.push(u);
            }
        }
    }
    return count;  // number of vertices reachable from start
}

static void BM_bfs_adjlist(benchmark::State& state) {
    int n = static_cast<int>(state.range(0));
    auto edges = random_edges(n, num_edges(n));
    auto graph = make_adjlist(n, edges);

    for (auto _ : state) {
        benchmark::DoNotOptimize(bfs(graph, 0));
    }
}

static void BM_bfs_csr(benchmark::State& state) {
    int n = static_cast<int>(state.range(0));
    auto edges = random_edges(n, num_edges(n));
    auto graph = make_csr(n, edges);

    for (auto _ : state) {
        benchmark::DoNotOptimize(bfs(graph, 0));
    }
}

// ============================================================
// Benchmark 3: Random vertex neighbor access
// ============================================================
// Access neighbors of randomly chosen vertices. This simulates the access
// pattern of algorithms like random walks or Monte Carlo graph methods.
// Stresses random-access latency more than sequential throughput.

static void BM_random_access_adjlist(benchmark::State& state) {
    int n = static_cast<int>(state.range(0));
    auto edges = random_edges(n, num_edges(n));
    auto graph = make_adjlist(n, edges);

    // Pre-generate random vertex sequence (outside measured loop).
    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dist(0, n - 1);
    std::vector<int> queries(10000);
    for (auto& q : queries) q = dist(rng);

    for (auto _ : state) {
        long sum = 0;
        for (int v : queries) {
            auto nbrs = graph.neighbors(v);
            if (!nbrs.empty()) sum += nbrs[0];  // touch first neighbor
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(queries.size()));
}

static void BM_random_access_csr(benchmark::State& state) {
    int n = static_cast<int>(state.range(0));
    auto edges = random_edges(n, num_edges(n));
    auto graph = make_csr(n, edges);

    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dist(0, n - 1);
    std::vector<int> queries(10000);
    for (auto& q : queries) q = dist(rng);

    for (auto _ : state) {
        long sum = 0;
        for (int v : queries) {
            auto nbrs = graph.neighbors(v);
            if (!nbrs.empty()) sum += nbrs[0];
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(queries.size()));
}

// ============================================================
// Register benchmarks
// ============================================================
// Sizes: 1k, 10k, 100k, 1M vertices.
// At avg degree 10, a 1M-vertex graph has ~5M edges → ~40 MB in CSR,
// larger in AdjacencyList due to per-vector overhead.

BENCHMARK(BM_iterate_neighbors_adjlist)->RangeMultiplier(10)->Range(1000, 1000000);
BENCHMARK(BM_iterate_neighbors_csr)->RangeMultiplier(10)->Range(1000, 1000000);

BENCHMARK(BM_bfs_adjlist)->RangeMultiplier(10)->Range(1000, 1000000);
BENCHMARK(BM_bfs_csr)->RangeMultiplier(10)->Range(1000, 1000000);

BENCHMARK(BM_random_access_adjlist)->RangeMultiplier(10)->Range(1000, 1000000);
BENCHMARK(BM_random_access_csr)->RangeMultiplier(10)->Range(1000, 1000000);
