"""Generate Amdahl's law comparison chart (amdahl_comparison.png).

Two side-by-side horizontal bar charts:
  Case 1: 10x on 30% → 1.37x total
  Case 2: 2x on 90% → 1.82x total
"""

import matplotlib.pyplot as plt
import matplotlib
matplotlib.use("Agg")

BG = "none"
FG = "#e0e0e0"
BLUE = "#4ea8de"
RED = "#e74c3c"

plt.rcParams.update({
    "figure.facecolor": BG,
    "axes.facecolor": BG,
    "text.color": FG,
    "axes.labelcolor": FG,
    "xtick.color": FG,
    "ytick.color": FG,
})

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 2.0))

cases = [
    {
        "title": "Case 1: 10x on 30% → 1.37x total",
        "hotspot_before": 0.30,
        "other_before": 0.70,
        "speedup": 10,
    },
    {
        "title": "Case 2: 2x on 90% → 1.82x total",
        "hotspot_before": 0.90,
        "other_before": 0.10,
        "speedup": 2,
    },
]

for ax, c in zip([ax1, ax2], cases):
    h_before = c["hotspot_before"]
    o_before = c["other_before"]
    h_after = h_before / c["speedup"]
    total_after = o_before + h_after

    labels = ["Before", "After"]
    other_vals = [o_before, o_before]
    hot_vals = [h_before, h_after]

    ax.barh(labels, other_vals, color=BLUE, label=f"Other ({o_before:.0%})")
    ax.barh(labels, hot_vals, left=other_vals, color=RED, label=f"Hotspot ({h_before:.0%})")

    ax.set_xlim(0, 1.05)
    ax.set_xlabel("Fraction of runtime")
    ax.set_title(c["title"], fontsize=11, fontweight="bold", color=FG)
    ax.legend(loc="upper right", fontsize=8, facecolor=(0, 0, 0, 0.5), edgecolor=FG,
              labelcolor=FG)
    ax.invert_yaxis()
    for spine in ax.spines.values():
        spine.set_visible(False)

plt.tight_layout()
plt.savefig("amdahl_comparison.png", dpi=200, bbox_inches="tight",
            transparent=True, edgecolor="none")
print("Saved amdahl_comparison.png")
