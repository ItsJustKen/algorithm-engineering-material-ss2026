"""
Example: Using cProfile to find hot functions and unexpected call counts.

Profiles the same naive graph-coloring solver from example 01, but instead
of reading log timestamps, we use cProfile to get exact call counts and
call-path structure. The key insight: call counts immediately reveal
algorithmic problems (e.g., a function called 800k times).

Treat the timing numbers as approximate: tracing adds overhead.

Three usage patterns are shown:
  1. Command-line:        python -m cProfile -s tottime profile_solver.py
  2. Context manager:     cProfile.Profile() around a specific section
  3. pstats post-analysis: reload a saved .prof file and query it

Run:
    pip install networkx ortools
    python profile_solver.py

Dependencies:
    networkx, ortools (same as example 01)
"""

import cProfile
import pstats
import sys

import networkx as nx
from ortools.sat.python import cp_model


# ---------------------------------------------------------------------------
# The solver (same logic as example 01, without logging)
# ---------------------------------------------------------------------------

def enumerate_all_cliques(G: nx.Graph):
    """Brute-force enumerate every clique of size >= 2."""
    adj = {v: set(G.neighbors(v)) for v in G.nodes()}
    nodes = sorted(G.nodes())

    def _extend(clique, candidates):
        for i, v in enumerate(candidates):
            new_clique = clique + [v]
            if len(new_clique) >= 2:
                yield list(new_clique)
            new_candidates = [u for u in candidates[i + 1 :] if u in adj[v]]
            yield from _extend(new_clique, new_candidates)

    yield from _extend([], nodes)


def build_model(G: nx.Graph, n_colors: int, cliques: list):
    """Build a CP-SAT model with all_different on each clique."""
    model = cp_model.CpModel()
    n = G.number_of_nodes()
    color = [model.new_int_var(0, n_colors - 1, f"c{v}") for v in range(n)]
    for clique in cliques:
        model.add_all_different([color[v] for v in clique])
    return model


def solve_coloring(G: nx.Graph) -> None:
    greedy = nx.coloring.greedy_color(G, strategy="largest_first")
    n_colors = max(greedy.values()) + 1

    cliques = list(enumerate_all_cliques(G))

    model = build_model(G, n_colors, cliques)

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = 60.0
    solver.solve(model)


# ---------------------------------------------------------------------------
# Profiling demonstrations
# ---------------------------------------------------------------------------

def demo_context_manager(G: nx.Graph):
    """Pattern 1: Profile a specific code section with the context manager."""
    print("=" * 60)
    print("DEMO: cProfile.Profile() context manager")
    print("=" * 60)

    with cProfile.Profile() as pr:
        solve_coloring(G)

    # Print the top 15 functions sorted by self-time
    print("\n--- Top 15 by tottime (self-time) ---")
    pr.print_stats(sort="tottime")

    # Save for later analysis
    pr.dump_stats("profile.prof")
    print("Saved profile to profile.prof\n")


def demo_pstats():
    """Pattern 2: Reload a saved profile and query it with pstats."""
    print("=" * 60)
    print("DEMO: pstats analysis of saved profile")
    print("=" * 60)

    p = pstats.Stats("profile.prof")

    # Top 10 by cumulative time (finds the top-level bottleneck)
    print("\n--- Top 10 by cumulative time ---")
    p.sort_stats("cumulative").print_stats(10)

    # Who calls enumerate_all_cliques's inner _extend function?
    print("\n--- Callers of '_extend' ---")
    p.print_callers("_extend")

    # What does _extend call?
    print("\n--- Callees of '_extend' ---")
    p.print_callees("_extend")


if __name__ == "__main__":
    G = nx.erdos_renyi_graph(25, 0.9, seed=42)

    if "--cli-demo" in sys.argv:
        # When run via: python -m cProfile -s tottime profile_solver.py --cli-demo
        # cProfile profiles the entire script including imports.
        solve_coloring(G)
    else:
        # Default: demonstrate the programmatic API
        demo_context_manager(G)
        demo_pstats()

        print("\n" + "=" * 60)
        print("TIP: You can also profile from the command line:")
        print("  python -m cProfile -s tottime profile_solver.py --cli-demo")
        print("  python -m cProfile -s cumtime profile_solver.py --cli-demo")
        print("  python -m cProfile -o prof.dat profile_solver.py --cli-demo")
        print("=" * 60)
