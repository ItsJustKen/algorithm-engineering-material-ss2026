"""Generate adjacency list vs CSR benchmark chart (gbench_adjlist_vs_csr.png).

Log-log plot of neighbor iteration time vs number of vertices.
Data from Google Benchmark output (Ryzen 9 9900X, GCC 15, -O2).
"""

import matplotlib.pyplot as plt
import numpy as np
import matplotlib
matplotlib.use("Agg")

BG = "none"
FG = "#e0e0e0"

plt.rcParams.update({
    "figure.facecolor": BG,
    "axes.facecolor": BG,
    "text.color": FG,
    "axes.labelcolor": FG,
    "xtick.color": FG,
    "ytick.color": FG,
    "axes.edgecolor": FG,
    "grid.color": "#333355",
    "grid.alpha": 0.5,
})

# Benchmark data (ns → ms)
N = np.array([1_000, 10_000, 100_000, 1_000_000])
adjlist_ns = np.array([1_469, 14_200, 830_000, 14_541_591])
csr_ns = np.array([1_475, 13_800, 620_000, 8_369_209])

adjlist_ms = adjlist_ns / 1e6
csr_ms = csr_ns / 1e6

fig, ax = plt.subplots(figsize=(8, 3.5))

ax.plot(N, adjlist_ms, "o-", color="#e74c3c", label="AdjacencyList",
        markersize=7, linewidth=2)
ax.plot(N, csr_ms, "s-", color="#2ecc71", label="CSR",
        markersize=7, linewidth=2)

ax.set_xscale("log")
ax.set_yscale("log")
ax.set_xlabel("Number of vertices (N)")
ax.set_ylabel("Time per iteration (ms)")
ax.set_title("Neighbor iteration: AdjacencyList vs CSR",
             fontsize=12, fontweight="bold", color=FG)
ax.legend(facecolor=(0, 0, 0, 0.5), edgecolor=FG, labelcolor=FG)
ax.grid(True, which="both", linestyle="--", alpha=0.3)

# Annotate "same (fits in cache)" at N=1000
ax.annotate("same\n(fits in cache)", xy=(1000, adjlist_ms[0]),
            xytext=(2500, adjlist_ms[0] * 0.4),
            fontsize=8, color=FG, fontstyle="italic",
            arrowprops=dict(arrowstyle="->", color=FG, lw=0.8))

# Annotate "1.7x" at N=1M
ratio = adjlist_ms[-1] / csr_ms[-1]
ax.annotate(f"{ratio:.1f}x", xy=(N[-1], csr_ms[-1]),
            xytext=(N[-1] * 0.3, csr_ms[-1] * 1.5),
            fontsize=12, color="#2ecc71", fontweight="bold",
            arrowprops=dict(arrowstyle="->", color="#2ecc71", lw=1.2))

for spine in ax.spines.values():
    spine.set_color(FG)

plt.tight_layout()
plt.savefig("gbench_adjlist_vs_csr.png", dpi=200, bbox_inches="tight",
            transparent=True, edgecolor="none")
print("Saved gbench_adjlist_vs_csr.png")
