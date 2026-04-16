/**
 * C++ Performance Killers — five common patterns with fast fixes.
 *
 * What: Benchmarks the slow vs fast version of five recurring C++ anti-patterns.
 * Why:  These are the patterns from the T01 "C++ Performance Killers" slides.
 *       The numbers shown in the slides come from this file.
 * How:  ./build/bm_cpp_killers
 *       ./build/bm_cpp_killers --benchmark_filter=copy   # one pattern only
 * When: Update N values if the slide examples change.
 *
 * Patterns:
 *   1. Accidental copies:  `auto r` vs `const auto& r`            (~24x)
 *   2. Missing reserve():  push_back without pre-allocation        (~2.3x)
 *   3. Erase-in-loop:      O(n²) erase vs erase-remove idiom      (>2000x at N=200k)
 *   4. AoS vs SoA:         32-byte struct, reading one field       (~2.3x)
 *   5. Dedup:              O(n²) nested loop vs sort+unique        (~90x)
 *
 * Note: Slides quote slightly different numbers — those were from an earlier run
 * and rounded for presentation. The ratios here reflect measured output on this
 * machine (AMD Ryzen 9 9900X, GCC, Release mode). The erase ratio in
 * particular is N-dependent (O(n²) vs O(n)) and grows with larger inputs.
 */

#include <algorithm>
#include <cstdint>
#include <list>
#include <numeric>
#include <random>
#include <set>
#include <vector>

#include <benchmark/benchmark.h>

// ============================================================
// Pattern 1: Accidental copies
// ============================================================
// `auto r` in a range-for copies the element — including its heap-allocated
// fields. For a struct with a std::vector payload, that means one
// heap allocation + memcpy + destructor per loop iteration.
// `const auto& r` takes a reference: zero cost.

struct Record {
    std::vector<int> payload;
};

static void BM_copy_by_value(benchmark::State& state) {
    int N = state.range(0);
    std::vector<Record> records(N);
    for (auto& r : records) r.payload.resize(32);  // 32 ints = 128 bytes

    for (auto _ : state) {
        size_t sum = 0;
        for (auto r : records)          // copies vector: alloc + memcpy + free
            sum += r.payload.size();
        benchmark::DoNotOptimize(sum);
    }
}

static void BM_copy_by_ref(benchmark::State& state) {
    int N = state.range(0);
    std::vector<Record> records(N);
    for (auto& r : records) r.payload.resize(32);

    for (auto _ : state) {
        size_t sum = 0;
        for (const auto& r : records)   // reference: zero overhead
            sum += r.payload.size();
        benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(BM_copy_by_value)->Arg(100'000);
BENCHMARK(BM_copy_by_ref) ->Arg(100'000);

// ============================================================
// Pattern 2: Missing reserve()
// ============================================================
// Without reserve(), push_back triggers ~log2(N) reallocations.
// Each reallocation allocates a larger buffer, copies all existing
// elements, then frees the old buffer.
// With reserve(N), a single allocation is made upfront.

static void BM_push_back_no_reserve(benchmark::State& state) {
    int N = state.range(0);
    for (auto _ : state) {
        std::vector<int> v;
        for (int i = 0; i < N; i++) v.push_back(i);
        benchmark::DoNotOptimize(v.data());
    }
}

static void BM_push_back_reserve(benchmark::State& state) {
    int N = state.range(0);
    for (auto _ : state) {
        std::vector<int> v;
        v.reserve(N);
        for (int i = 0; i < N; i++) v.push_back(i);
        benchmark::DoNotOptimize(v.data());
    }
}

BENCHMARK(BM_push_back_no_reserve)->Arg(10'000'000);
BENCHMARK(BM_push_back_reserve)   ->Arg(10'000'000);

// ============================================================
// Pattern 3: Erase-in-loop (O(n²) hiding in plain sight)
// ============================================================
// v.erase(it) shifts all elements after the erased position by one.
// In a loop that erases ~33% of elements, this becomes O(n²).
// The erase-remove idiom does a single O(n) pass instead.

static std::vector<int> make_erase_data(int N) {
    std::vector<int> v(N);
    for (int i = 0; i < N; i++) v[i] = (i % 3 == 0) ? -(i + 1) : i;  // ~33% negative
    return v;
}

static void BM_erase_in_loop(benchmark::State& state) {
    int N = state.range(0);
    auto data = make_erase_data(N);
    for (auto _ : state) {
        auto v = data;
        for (auto it = v.begin(); it != v.end();) {
            if (*it < 0) it = v.erase(it);  // O(n) shift per erase → O(n²) total
            else         ++it;
        }
        benchmark::DoNotOptimize(v.data());
    }
}

static void BM_erase_remove(benchmark::State& state) {
    int N = state.range(0);
    auto data = make_erase_data(N);
    for (auto _ : state) {
        auto v = data;
        v.erase(                                    // single O(n) pass
            std::remove_if(v.begin(), v.end(), [](int x) { return x < 0; }),
            v.end());
        benchmark::DoNotOptimize(v.data());
    }
}

// Note: BM_erase_in_loop at N=200k is O(n²) and takes ~100–300 ms per iteration.
// Google Benchmark will run only a few iterations, which is expected.
BENCHMARK(BM_erase_in_loop)->Arg(200'000);
BENCHMARK(BM_erase_remove) ->Arg(200'000);

// ============================================================
// Pattern 4: AoS vs SoA — memory layout for field access
// ============================================================
// Particle is 32 bytes. Accessing only `x` means 7/8 of each loaded
// cache line is wasted bandwidth. SoA packs `x` contiguously.

struct Particle {    // 32 bytes total
    float x, y, z;  // position: 12 bytes
    float vx, vy, vz;  // velocity: 12 bytes
    uint32_t id, flags; // metadata: 8 bytes
};

struct ParticlesSoA {
    std::vector<float> x, y, z;
    std::vector<float> vx, vy, vz;
    std::vector<uint32_t> id, flags;

    explicit ParticlesSoA(int n) : x(n), y(n), z(n), vx(n), vy(n), vz(n), id(n), flags(n) {}
};

static void BM_aos_sum_x(benchmark::State& state) {
    int N = state.range(0);
    std::vector<Particle> ps(N);
    for (int i = 0; i < N; i++) ps[i].x = float(i);

    for (auto _ : state) {
        float s = 0;
        for (const auto& p : ps) s += p.x;
        benchmark::DoNotOptimize(s);
    }
    state.SetItemsProcessed(state.iterations() * N);
}

static void BM_soa_sum_x(benchmark::State& state) {
    int N = state.range(0);
    ParticlesSoA ps(N);
    for (int i = 0; i < N; i++) ps.x[i] = float(i);

    for (auto _ : state) {
        float s = 0;
        for (float xi : ps.x) s += xi;
        benchmark::DoNotOptimize(s);
    }
    state.SetItemsProcessed(state.iterations() * N);
}

BENCHMARK(BM_aos_sum_x)->Arg(10'000'000);
BENCHMARK(BM_soa_sum_x)->Arg(10'000'000);

// ============================================================
// Pattern 5: Accidental O(n²) — dedup with nested loop
// ============================================================
// For each input element, the naive dedup scans the entire output
// vector for duplicates: O(n) per element → O(n²) total.
// Sort + unique is O(n log n) and uses only standard library calls.

static std::vector<int> make_dedup_data(int N) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, N / 2 - 1);
    std::vector<int> v(N);
    for (auto& x : v) x = dist(rng);  // ~50% duplicates
    return v;
}

static void BM_dedup_naive(benchmark::State& state) {
    int N = state.range(0);
    auto data = make_dedup_data(N);
    for (auto _ : state) {
        std::vector<int> out;
        for (int x : data) {
            bool seen = false;
            for (int y : out)
                if (y == x) { seen = true; break; }
            if (!seen) out.push_back(x);
        }
        benchmark::DoNotOptimize(out.data());
    }
}

static void BM_dedup_sort_unique(benchmark::State& state) {
    int N = state.range(0);
    auto data = make_dedup_data(N);
    for (auto _ : state) {
        auto v = data;
        std::sort(v.begin(), v.end());
        v.erase(std::unique(v.begin(), v.end()), v.end());
        benchmark::DoNotOptimize(v.data());
    }
}

// Note: BM_dedup_naive at N=100k is O(n²) — expect ~200–400 ms per iteration.
BENCHMARK(BM_dedup_naive)      ->Arg(100'000);
BENCHMARK(BM_dedup_sort_unique)->Arg(100'000);

// ============================================================
// Pattern 6: Pointer chasing vs contiguous storage
// ============================================================
// A linked list scatters nodes across the heap. Traversing it means
// following a pointer on every step — each likely a cache miss.
// A flat vector stores elements contiguously; the hardware prefetcher
// streams them into cache automatically.
//
// This is the purest form of the "data layout matters" lesson.
// AoS vs SoA (pattern 4) is a subtler version of the same effect.

// Sum all values in a std::list (heap-scattered nodes, pointer chasing)
static void BM_sum_linked_list(benchmark::State& state) {
    int N = state.range(0);
    std::list<int> lst;
    for (int i = 0; i < N; i++) lst.push_back(i);

    for (auto _ : state) {
        long sum = 0;
        for (int x : lst) sum += x;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * N);
}

// Sum all values in a std::vector (contiguous, prefetcher-friendly)
static void BM_sum_vector(benchmark::State& state) {
    int N = state.range(0);
    std::vector<int> vec(N);
    std::iota(vec.begin(), vec.end(), 0);

    for (auto _ : state) {
        long sum = 0;
        for (int x : vec) sum += x;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * N);
}

// Linked list with shuffled allocation order — worst-case pointer chasing.
// Nodes are allocated in random order so they are not accidentally
// sequential in the heap allocator's free list.
static void BM_sum_linked_list_shuffled(benchmark::State& state) {
    int N = state.range(0);

    // Create nodes in random order to scatter them across the heap
    std::vector<int> order(N);
    std::iota(order.begin(), order.end(), 0);
    std::mt19937 rng(42);
    std::shuffle(order.begin(), order.end(), rng);

    std::list<int> lst;
    // Insert in shuffled order, then sort to get sequential values
    // but physically scattered nodes
    for (int i : order) lst.push_back(i);
    lst.sort();

    for (auto _ : state) {
        long sum = 0;
        for (int x : lst) sum += x;
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * N);
}

BENCHMARK(BM_sum_linked_list)         ->Arg(1'000'000);
BENCHMARK(BM_sum_linked_list_shuffled)->Arg(1'000'000);
BENCHMARK(BM_sum_vector)              ->Arg(1'000'000);

// ============================================================
// Pattern 7: std::set vs sorted std::vector for lookup
// ============================================================
// In Python, set beats list for membership tests — always.
// In C++, it's more nuanced: std::set is a red-black tree with
// pointer-chasing on every lookup. A sorted std::vector with
// binary_search keeps data contiguous and cache-friendly.
//
// For small to moderate sizes, the vector wins despite the same
// O(log n) complexity — because cache misses dominate.
// At very large sizes, the tree may catch up (or not).

static std::vector<int> make_lookup_queries(int N, int n_queries, unsigned seed = 123) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, N * 2);  // ~50% hit rate
    std::vector<int> queries(n_queries);
    for (auto& q : queries) q = dist(rng);
    return queries;
}

static void BM_lookup_set(benchmark::State& state) {
    int N = state.range(0);
    int Q = 100'000;
    std::set<int> s;
    for (int i = 0; i < N; i++) s.insert(i);
    auto queries = make_lookup_queries(N, Q);

    for (auto _ : state) {
        int count = 0;
        for (int q : queries) count += s.count(q);
        benchmark::DoNotOptimize(count);
    }
    state.SetItemsProcessed(state.iterations() * Q);
}

static void BM_lookup_sorted_vector(benchmark::State& state) {
    int N = state.range(0);
    int Q = 100'000;
    std::vector<int> v(N);
    std::iota(v.begin(), v.end(), 0);  // already sorted
    auto queries = make_lookup_queries(N, Q);

    for (auto _ : state) {
        int count = 0;
        for (int q : queries)
            count += std::binary_search(v.begin(), v.end(), q);
        benchmark::DoNotOptimize(count);
    }
    state.SetItemsProcessed(state.iterations() * Q);
}

// Linear scan — only competitive at small N, included for reference
static void BM_lookup_unsorted_vector(benchmark::State& state) {
    int N = state.range(0);
    int Q = 100'000;
    std::vector<int> v(N);
    std::iota(v.begin(), v.end(), 0);
    // Shuffle so we can't exit early on average
    std::mt19937 rng(42);
    std::shuffle(v.begin(), v.end(), rng);
    auto queries = make_lookup_queries(N, Q);

    for (auto _ : state) {
        int count = 0;
        for (int q : queries)
            count += (std::find(v.begin(), v.end(), q) != v.end());
        benchmark::DoNotOptimize(count);
    }
    state.SetItemsProcessed(state.iterations() * Q);
}

// Sweep across sizes to show the crossover point
BENCHMARK(BM_lookup_set)             ->RangeMultiplier(10)->Range(100, 10'000'000);
BENCHMARK(BM_lookup_sorted_vector)   ->RangeMultiplier(10)->Range(100, 10'000'000);
BENCHMARK(BM_lookup_unsorted_vector) ->RangeMultiplier(10)->Range(100, 1'000'000);
