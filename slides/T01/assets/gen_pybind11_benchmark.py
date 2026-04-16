"""Generate pybind11 benchmark chart (pybind11_benchmark.png).

Grouped bar chart: Python vs C++ (on-the-fly) vs C++ (precomputed)
for TSP N=1000.
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
})

categories = ["Nearest\nNeighbor", "2-opt", "Total"]
python = [0.059, 1.259, 1.318]
cpp_otf = [0.001, 0.007, 0.007]
cpp_pre = [None, None, 0.005]  # only total for precomputed

x = np.arange(len(categories))
width = 0.25

fig, ax = plt.subplots(figsize=(6, 3.7))

bars_py = ax.bar(x - width, python, width, color="#e74c3c", label="Python")
bars_cpp = ax.bar(x, cpp_otf, width, color="#4ea8de", label="C++ (on-the-fly)")

# Precomputed only has total
bars_pre = ax.bar(x[2] + width, cpp_pre[2], width, color="#2ecc71",
                  label="C++ (precomputed)")

# Value labels on bars
for bar, val in zip(bars_py, python):
    ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.02,
            f"{val:.3f}s", ha="center", va="bottom", fontsize=8, color=FG)

for bar, val in zip(bars_cpp, cpp_otf):
    ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + 0.02,
            f"{val:.3f}s", ha="center", va="bottom", fontsize=8, color=FG)

ax.text(bars_pre.patches[0].get_x() + bars_pre.patches[0].get_width() / 2,
        cpp_pre[2] + 0.02, f"{cpp_pre[2]:.3f}s",
        ha="center", va="bottom", fontsize=8, color=FG)

# Speedup annotations
# 57x arrow from Python total to C++ on-the-fly total
ax.annotate("190x", xy=(x[2], cpp_otf[2] + 0.01),
            xytext=(x[2] + 0.35, python[2] * 0.55),
            fontsize=13, color="#4ea8de", fontweight="bold",
            arrowprops=dict(arrowstyle="->", color="#4ea8de", lw=1.5))

# 260x arrow from Python total to C++ precomputed total
ax.annotate("260x", xy=(x[2] + width, cpp_pre[2] + 0.01),
            xytext=(x[2] + width + 0.3, python[2] * 0.35),
            fontsize=13, color="#2ecc71", fontweight="bold",
            arrowprops=dict(arrowstyle="->", color="#2ecc71", lw=1.5))

ax.set_ylabel("Time (seconds)")
ax.set_title("TSP (N=1000): Python vs C++ via pybind11",
             fontsize=12, fontweight="bold", color=FG)
ax.set_xticks(x)
ax.set_xticklabels(categories)
ax.legend(facecolor=(0, 0, 0, 0.5), edgecolor=FG, labelcolor=FG, loc="upper left")

for spine in ax.spines.values():
    spine.set_color(FG)

plt.tight_layout()
plt.savefig("pybind11_benchmark.png", dpi=200, bbox_inches="tight",
            transparent=True, edgecolor="none")
print("Saved pybind11_benchmark.png")
