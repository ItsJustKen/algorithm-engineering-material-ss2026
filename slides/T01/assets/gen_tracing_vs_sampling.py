"""Generate tracing vs sampling diagram (tracing_vs_sampling.png).

Timeline showing function calls with tracing markers (every call) vs
sampling markers (periodic snapshots).
"""

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib
matplotlib.use("Agg")

BG = "none"
FG = "#e0e0e0"
COLORS = {"parse_input": "#4ea8de", "build_graph": "#e74c3c",
          "gap": "#2ecc71", "solve": "#e74c3c"}

plt.rcParams.update({
    "figure.facecolor": BG,
    "axes.facecolor": BG,
    "text.color": FG,
    "axes.labelcolor": FG,
    "xtick.color": FG,
    "ytick.color": FG,
})

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 3.5), sharex=True)

# Function calls timeline
calls = [
    ("parse_input", 0.0, 0.8),
    ("build_graph", 0.8, 2.6),
    ("gap", 2.6, 2.8),
    ("solve", 2.8, 6.5),
]

bar_h = 0.5
y_bar = 0.3
marker_y = y_bar - bar_h / 2 - 0.12  # just below the bars
label_y = marker_y - 0.18             # text below markers

for ax in [ax1, ax2]:
    for name, start, end in calls:
        color = COLORS[name]
        ax.barh(y_bar, end - start, left=start, height=bar_h, color=color,
                edgecolor="none")
        if name != "gap":
            ax.text((start + end) / 2, y_bar, name.replace("_", "_"),
                    ha="center", va="center", fontsize=8, color="white",
                    fontfamily="monospace", fontweight="bold")
    for spine in ax.spines.values():
        spine.set_visible(False)
    ax.set_yticks([])
    ax.set_ylim(-0.55, 1.0)

# Tracing: markers at every call boundary
ax1.text(-0.15, 0.88, "Tracing: record every call", fontsize=11,
         fontweight="bold", color=FG, transform=ax1.transAxes)
trace_times = [0.0, 0.8, 0.8, 2.6, 2.6, 2.8, 2.8, 6.5]
for t in trace_times:
    ax1.plot(t, marker_y, marker="|", color="#f39c12", markersize=10, markeredgewidth=2)
ax1.text(3.3, label_y, "every call entry/exit recorded", fontsize=8,
         color="#f39c12", fontstyle="italic", ha="center")
ax1.tick_params(axis="x", which="both", length=0, labelbottom=False)

# Sampling: periodic markers (triangles pointing up into bars)
ax2.text(-0.15, 0.88, "Sampling: periodic snapshots", fontsize=11,
         fontweight="bold", color=FG, transform=ax2.transAxes)
import numpy as np
sample_times = np.arange(0.3, 6.5, 0.7)
for t in sample_times:
    ax2.plot(t, marker_y, marker="^", color="#f39c12", markersize=7)
ax2.text(3.3, label_y, "periodic interrupts — statistical picture", fontsize=8,
         color="#f39c12", fontstyle="italic", ha="center")

ax2.set_xlabel("Time →")
ax2.set_xticks(range(7))
ax2.set_xlim(-0.2, 7)

plt.tight_layout()
plt.savefig("tracing_vs_sampling.png", dpi=200, bbox_inches="tight",
            transparent=True, edgecolor="none")
print("Saved tracing_vs_sampling.png")
