"""Generate AoS vs SoA cache layout diagram (aos_vs_soa.png).

Shows how AoS loads irrelevant fields per cache line, while SoA packs
only the accessed field contiguously.
"""

import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib
matplotlib.use("Agg")

BG = "none"
FG = "#e0e0e0"
USED = "#e74c3c"
UNUSED = "#3a3a5c"
CELL_W = 1.0
CELL_H = 0.7
GAP = 0.15

plt.rcParams.update({
    "figure.facecolor": BG,
    "axes.facecolor": BG,
    "text.color": FG,
})

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 4.0))

# -- AoS --
ax1.set_title("AoS: Array of Structs — reading only x loads everything",
              fontsize=11, fontweight="bold", color=FG, loc="left")

fields = ["x", "y", "z", "vx", "vy", "vz", "id", "fl"]
n_structs = 3
x_pos = 0
cache_line_positions = []
for s in range(n_structs):
    start_x = x_pos
    for i, f in enumerate(fields):
        color = USED if f == "x" else UNUSED
        rect = mpatches.FancyBboxPatch((x_pos, 0), CELL_W - GAP, CELL_H,
                                        boxstyle="round,pad=0.05",
                                        facecolor=color, edgecolor=FG,
                                        linewidth=0.5)
        ax1.add_patch(rect)
        ax1.text(x_pos + (CELL_W - GAP) / 2, CELL_H / 2, f,
                 ha="center", va="center", fontsize=7, color="white",
                 fontfamily="monospace")
        x_pos += CELL_W
    if s == 0:
        cache_line_positions.append((start_x, x_pos))
    if s < n_structs - 1:
        x_pos += 0.3  # gap between structs

# cache line bracket for first struct
cl_start, cl_end = cache_line_positions[0]
bracket_y = -0.35
ax1.annotate("", xy=(cl_start, bracket_y), xytext=(cl_end - GAP, bracket_y),
             arrowprops=dict(arrowstyle="<->", color=FG, lw=1.2))
ax1.text((cl_start + cl_end - GAP) / 2, bracket_y - 0.2, "cache line",
         ha="center", va="top", fontsize=8, color=FG, fontstyle="italic")

# second cache line bracket
if n_structs >= 2:
    cl2_start = cache_line_positions[0][1] + 0.3
    cl2_end = cl2_start + len(fields) * CELL_W
    ax1.annotate("", xy=(cl2_start, bracket_y), xytext=(cl2_end - GAP, bracket_y),
                 arrowprops=dict(arrowstyle="<->", color=FG, lw=1.2))
    ax1.text((cl2_start + cl2_end - GAP) / 2, bracket_y - 0.2, "cache line",
             ha="center", va="top", fontsize=8, color=FG, fontstyle="italic")

# ellipsis
ax1.text(x_pos + 0.5, CELL_H / 2, "...", fontsize=16, color=FG,
         ha="center", va="center")

ax1.set_xlim(-0.3, x_pos + 1.5)
ax1.set_ylim(-0.8, 1.2)
ax1.axis("off")

# -- SoA --
ax2.set_title("SoA: Struct of Arrays — reading x loads only x values",
              fontsize=11, fontweight="bold", color=FG, loc="left")

n_elements = 16
x_pos = 0
soa_start = 0
for i in range(n_elements):
    rect = mpatches.FancyBboxPatch((x_pos, 0), CELL_W - GAP, CELL_H,
                                    boxstyle="round,pad=0.05",
                                    facecolor=USED, edgecolor=FG,
                                    linewidth=0.5)
    ax2.add_patch(rect)
    ax2.text(x_pos + (CELL_W - GAP) / 2, CELL_H / 2, "x",
             ha="center", va="center", fontsize=7, color="white",
             fontfamily="monospace")
    x_pos += CELL_W

# cache line bracket spanning ~8 elements
bracket_y = -0.35
ax2.annotate("", xy=(soa_start, bracket_y),
             xytext=(8 * CELL_W - GAP, bracket_y),
             arrowprops=dict(arrowstyle="<->", color=FG, lw=1.2))
ax2.text((8 * CELL_W - GAP) / 2, bracket_y - 0.2, "cache line (all useful!)",
         ha="center", va="top", fontsize=8, color=FG, fontstyle="italic")

ax2.text(x_pos + 0.3, CELL_H / 2, "...", fontsize=16, color=FG,
         ha="center", va="center")

ax2.set_xlim(-0.3, x_pos + 1.0)
ax2.set_ylim(-0.8, 1.2)
ax2.axis("off")

plt.tight_layout()
plt.savefig("aos_vs_soa.png", dpi=200, bbox_inches="tight",
            transparent=True, edgecolor="none")
print("Saved aos_vs_soa.png")
