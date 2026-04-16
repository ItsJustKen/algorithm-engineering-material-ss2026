"""Generate Python killers summary chart (python_killers_summary.png).

Horizontal bar chart (log scale) of measured speedups for Python fixes.
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

# Data from slides (order: bottom to top in chart)
fixes = [
    ("Python loops\n→ NumPy", 5_000, "#9b59b6"),
    ("@lru_cache", 50_000, "#2ecc71"),
    ("loop → \nsum()", 3.9, "#2ecc71"),
    ("str += →\n.join()", 18, "#f39c12"),
    ("list → set\nmembership", 1_034, "#4ea8de"),
]

labels = [f[0] for f in fixes]
speedups = [f[1] for f in fixes]
colors = [f[2] for f in fixes]

fig, ax = plt.subplots(figsize=(8, 3.5))

y = np.arange(len(fixes))
bars = ax.barh(y, speedups, color=colors, height=0.6, edgecolor="none")

ax.set_xscale("log")
ax.set_xlabel("Speedup (log scale)")
ax.set_yticks(y)
ax.set_yticklabels(labels, fontsize=9)
ax.set_title("Python Performance Killers — Measured Speedups",
             fontsize=12, fontweight="bold", color=FG)

# Value labels
for bar, val in zip(bars, speedups):
    label = f"{val:,.0f}x" if val >= 10 else f"{val:.1f}x"
    ax.text(bar.get_width() * 1.3, bar.get_y() + bar.get_height() / 2,
            label, ha="left", va="center", fontsize=11, fontweight="bold",
            color=FG)

ax.set_xlim(1, 200_000)
for spine in ["top", "right"]:
    ax.spines[spine].set_visible(False)
for spine in ["bottom", "left"]:
    ax.spines[spine].set_color(FG)
ax.grid(axis="x", linestyle="--", alpha=0.2, color=FG)

plt.tight_layout()
plt.savefig("python_killers_summary.png", dpi=200, bbox_inches="tight",
            transparent=True, edgecolor="none")
print("Saved python_killers_summary.png")
