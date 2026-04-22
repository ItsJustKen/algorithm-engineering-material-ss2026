#include <cstddef>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <random>
#include <chrono>
#include <ranges>
#include <numeric>
#include <cassert>
#include <nlohmann/json.hpp>

using Index = std::size_t;

/**
 * You are free to change the graph representation,
 * if you want, as long as the behavior of the program remains identical,
 * i.e., the probability with which a certain clique is selected is unchanged,
 * (under the idealized assumption that the RNG behaves truly randomly
 * instead of having a fixed seed).
 */
class Graph {
public:
    explicit Graph(std::vector<std::vector<Index>> adj) : 
        m_n(adj.size()), 
        m_words_per_row((adj.size() + 63) / 64) 
    {
        m_adj_matrix.assign(m_n * m_words_per_row, 0ULL);
        m_adj_list = std::move(adj);

        for (Index i = 0; i < m_n; ++i) {
            for (Index neighbor : m_adj_list[i]) {
                m_adj_matrix[i * m_words_per_row + (neighbor / 64)] |= (1ULL << (neighbor % 64));
            }
        }
    }

    inline const uint64_t* row(Index v) const {
        return &m_adj_matrix[v * m_words_per_row];
    }

    const std::vector<Index>& neighbors(Index v) const { return m_adj_list[v]; }
    std::size_t n() const { return m_n; }
    std::size_t words_per_row() const { return m_words_per_row; }

private:
    std::size_t m_n;
    std::size_t m_words_per_row;
    std::vector<uint64_t> m_adj_matrix;
    std::vector<std::vector<Index>> m_adj_list;
};

Graph load_graph(const std::filesystem::path& path) {
    std::ifstream input(path);
    auto parsed = nlohmann::json::parse(input);
    return Graph(parsed.get<std::vector<std::vector<Index>>>());
}

/**
 * Computes a greedy clique as follows:
 * - iterate through the vertices of the graph in random order,
 * - add the next vertex in that order if it is still 
 *   a viable extension of the current clique.
 */
class GreedyClique {
public:
    GreedyClique(const Graph* g) : 
        rng(1337), 
        graph(g), 
        order(g->n()), 
        possible_ext(g->words_per_row()) 
    {
        std::iota(order.begin(), order.end(), Index(0));
    }

    const std::vector<Index>& random_order_greedy_clique() {
        std::ranges::shuffle(order, rng);
        clique_members.clear();
        
        Index v0 = order[0];
        clique_members.push_back(v0);
        
        const uint64_t* v0_neighbors = graph->row(v0);
        std::copy(v0_neighbors, v0_neighbors + graph->words_per_row(), possible_ext.begin());

        for (std::size_t i = 1; i < order.size(); ++i) {
            Index v = order[i];
            
            if (possible_ext[v / 64] & (1ULL << (v % 64))) {
                clique_members.push_back(v);
                
                const uint64_t* v_neighbors = graph->row(v);
                bool any_left = false;
                for (std::size_t w = 0; w < possible_ext.size(); ++w) {
                    possible_ext[w] &= v_neighbors[w];
                    if (possible_ext[w]) any_left = true;
                }
                
                if (!any_left) break;
            }
        }
        return clique_members;
    }

private:
    const Graph* graph;
    std::mt19937_64 rng;
    std::vector<Index> order;
    std::vector<uint64_t> possible_ext;
    std::vector<Index> clique_members;
};
std::vector<Index> greedy_clique(const Graph& graph, std::size_t num_attempts) {
    GreedyClique greedy_clique(&graph);
    std::vector<Index> best_clique;
    for(std::size_t i = 0; i < num_attempts; ++i) {
        const auto& clique = greedy_clique.random_order_greedy_clique();
        if(clique.size() > best_clique.size()) {
            best_clique = clique;
        }
    }
    return best_clique;
}

void validate_clique(const Graph& graph, const std::vector<Index>& clique) {
    for(std::size_t i = 0; i < clique.size(); ++i) {
        for(std::size_t j = i + 1; j < clique.size(); ++j) {
            Index v1 = clique[i];
            Index v2 = clique[j];
            const auto& neighbors = graph.neighbors(v1);
            if(!std::binary_search(neighbors.begin(), neighbors.end(), v2)) {
                std::cerr << "You done goofed, the clique is invalid:\n";
                std::cerr << "vertices " << v1 << " and " << v2 
                          << " are not connected!\n";
                std::exit(1);
            }
        }
    }
}

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "Expected JSON graph file as single argument!\n";
        return 1;
    }

    Graph input_graph = load_graph(argv[1]);
    auto before = std::chrono::steady_clock::now();
    std::vector<Index> result = greedy_clique(input_graph, 50'000);
    auto after = std::chrono::steady_clock::now();
    std::cout << "Clique with " << result.size() << " vertices!\n";
    for(Index v : result) {
        std::cout << "\t" << v << std::endl;
    }
    std::cout << "Found in " << 
        std::chrono::duration_cast<std::chrono::duration<double>>(after - before).count() << " seconds.\n";
    validate_clique(input_graph, result);
    return 0;
}
