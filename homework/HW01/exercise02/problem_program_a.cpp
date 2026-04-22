#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <cstddef>
#include <tuple>
#include <utility>

/**
 * Determines the orientation of the triplet (p1, p2, p3):
 *   - returns >0 if left turn,
 *   - returns <0 if right turn,
 *   - returns ==0 if collinear.
 */
std::int64_t turn(std::array<std::int64_t, 2> p1,
                  std::array<std::int64_t, 2> p2,
                  std::array<std::int64_t, 2> p3) 
{
    return (p2[0] - p1[0]) * (p3[1] - p1[1]) - 
           (p2[1] - p1[1]) * (p3[0] - p1[0]);
}

/**
 * Generates test data for the problem;
 * you do NOT need to modify this function,
 * and it should not be included in the time measurement.
 */
[[gnu::noinline]] std::tuple<
    std::vector<std::size_t>,  // test array 'a'
    std::vector<std::size_t>,  // test array 'b'
    std::vector<double>        // test array 'c'
> generate_test_data(std::size_t n) {
    std::size_t m = n * n;
    std::mt19937_64 rng(n);
    std::uniform_int_distribution<std::size_t> dist(0, m - 1);
    std::uniform_real_distribution<double> dist_double(0.0, 1.0);
    std::vector<double> c;
    c.reserve(m);
    for(std::size_t i = 0; i < m; ++i) {
        c.push_back(dist_double(rng));
    }
    std::vector<std::size_t> a, b;
    a.reserve(n);
    b.reserve(n);
    for(std::size_t i = 0; i < n; ++i) {
        a.push_back(dist(rng));
        b.push_back(dist(rng));
    }
    return {std::move(a), std::move(b), std::move(c)};
}


[[gnu::noinline]]
double problematic_function(const std::vector<std::size_t>& a,
                            const std::vector<std::size_t>& b,
                            const std::vector<double>& c,
                            std::int64_t x, std::int64_t y) 
{
    std::size_t n = a.size();
    std::size_t m = n * n;
    double result = 0.0;
    for(std::size_t i = 0; i < n; ++i) {
        for(std::size_t j = 0; j < n; ++j) {
            std::size_t ax = i + j;
            if(i + j >= n) {
                ax -= n;
            }
            std::int64_t a_i = a[ax];
            std::int64_t b_j = b[j];
            auto t1 = turn({x, y}, {a_i, b_j}, {b_j, a_i});
            auto t2 = turn({x, y}, {-b_j, -a_i}, {-a_i, -b_j});
            bool mask = (t1 >= 0) && (t2 >= 0);
            result += mask * (c[(a_i + b_j) % m] * (t1 + t2 + 1));
        }
    }
    return result;
}


int main(int argc, char** argv) {
    std::size_t n = 32768;
    if(argc == 2) {
        n = static_cast<std::size_t>(std::stoull(argv[1]));
    }
    auto [a, b, c] = generate_test_data(n);
    auto before = std::chrono::high_resolution_clock::now();
    double result = problematic_function(a, b, c, n, n);
    auto after = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = after - before;
    std::cout << "Result: " << result << "\n";
    std::cout << "Elapsed time: " << elapsed.count() << " seconds\n";
    return 0;
}
