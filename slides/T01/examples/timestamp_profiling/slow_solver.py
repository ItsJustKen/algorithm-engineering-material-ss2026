"""
Example: Using basic logging to find performance bottlenecks.

A naive graph coloring solver that enumerates all cliques in a graph
and posts them as all_different constraints to CP-SAT.  The logging
timestamps immediately reveal that clique enumeration — not the
NP-hard solver — dominates the runtime.

Run:
    python slow_solver.py

Dependencies:
    pip install networkx ortools
"""

import logging

import networkx as nx
from ortools.sat.python import cp_model

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(message)s",
)
log = logging.getLogger(__name__)


def _enumerate_all_cliques(G: nx.Graph):
    """Enumerate every clique of size >= 2 by brute-force back-tracking.

    For each node we try extending the current clique with every
    candidate neighbor, yielding all sub-cliques along the way.
    This deliberately generates a huge number of (redundant) cliques
    to create an obvious bottleneck for profiling exercises.
    """
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


def solve_coloring(G: nx.Graph) -> None:
    log.info("Graph: nodes=%d  edges=%d", G.number_of_nodes(), G.number_of_edges())

    # --- Phase 1: determine number of colors via greedy upper bound ---
    log.info("Running greedy coloring for upper bound")
    greedy = nx.coloring.greedy_color(G, strategy="largest_first")
    n_colors = max(greedy.values()) + 1
    log.info("Greedy coloring done: %d colors", n_colors)

    # --- Phase 2: enumerate all cliques ---
    # This is the slow step.  The timestamp gap in the log makes that obvious.
    log.info("Enumerating all cliques...")
    cliques = list(_enumerate_all_cliques(G))
    log.info(
        "Cliques done: count=%d  max_size=%d",
        len(cliques),
        max(len(c) for c in cliques),
    )

    # --- Phase 3: build CP-SAT model ---
    log.info("Building CP-SAT model with all_different on each clique")
    model = cp_model.CpModel()
    n = G.number_of_nodes()
    color = [model.new_int_var(0, n_colors - 1, f"c{v}") for v in range(n)]

    for clique in cliques:
        model.add_all_different([color[v] for v in clique])

    log.info("Model built: %d variables, %d constraints", n, len(cliques))

    # --- Phase 4: solve ---
    log.info("Solving...")
    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = 60.0
    status = solver.solve(model)
    log.info("Solve done: status=%s  wall_time=%.2fs", solver.status_name(status), solver.wall_time)


if __name__ == "__main__":
    # Low max degree, but the number of cliques still blows up.
    G = nx.erdos_renyi_graph(25, 0.9, seed=42)
    solve_coloring(G)
