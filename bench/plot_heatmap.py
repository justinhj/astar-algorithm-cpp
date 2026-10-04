#!/usr/bin/env python3
import argparse
import os
import re
import matplotlib
matplotlib.use("Agg")  # Set headless backend before importing pyplot
import matplotlib.colors as mcolors
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

CUSTOM_COLORS = mcolors.LinearSegmentedColormap.from_list(
    "slate_teal",
    [
        (0.00, "#e76f51"),  # Burnt coral for severe regression
        (0.40, "#f4a261"),  # Soft peach for slight regression
        (0.50, "#fafaf9"),  # Neutral warm white / stone (1.0x)
        (0.65, "#a8dadc"),  # Soft cyan / ice blue (~2x)
        (0.85, "#457b9d"),  # Steel blue (~4x - 6x)
        (1.00, "#1d3557"),  # Deep navy (~10x+)
    ],
)

def parse_results(results_path="results.txt"):
    if not os.path.exists(results_path):
        raise FileNotFoundError(f"Could not find results file at {results_path}")

    with open(results_path, "r") as f:
        content = f.read()

    sections = re.split(r"BENCHMARK RUN: GRID_SIZE = (\d+)", content)
    grid_data = {}
    for i in range(1, len(sections), 2):
        g = int(sections[i])
        body = sections[i + 1]
        lines = body.splitlines()
        grid_data[g] = {}
        for line in lines:
            if line.startswith("bench_") or line.startswith("bench "):
                parts = [p.strip() for p in line.split("|")]
                if len(parts) >= 4:
                    desc = parts[2]
                    mean_match = re.search(r"([0-9.]+)\s+s", parts[3])
                    if mean_match:
                        grid_data[g][desc] = float(mean_match.group(1))
    return grid_data

def generate_heatmap(
    grid_data,
    output_path="benchmark_heatmap.png",
    cmap=None,
    use_log2=True,
    center_zero=True,
):
    if cmap is None:
        cmap = CUSTOM_COLORS

    grids = sorted(grid_data.keys())
    stages = [
        "Baseline (Pre-optimization)",
        "Step 1: Open set lookup O(1)",
        "Step 2: Intrusive min-heap (siftUp/siftDown)",
        "Step 3: Unified node map & deferred alloc",
        "Current master",
    ]

    speedup_matrix = []
    annot_matrix = []

    for stage in stages:
        speedup_row = []
        annot_row = []
        for g in grids:
            t = grid_data[g].get(stage, np.nan)
            base_t = grid_data[g]["Baseline (Pre-optimization)"]
            sp = base_t / t if t > 0 else 1.0
            speedup_row.append(sp)

            if t < 0.1:
                t_str = f"{t * 1000:.1f} ms"
            else:
                t_str = f"{t:.2f} s"
            annot_row.append(f"{sp:.1f}x\n({t_str})")

        speedup_matrix.append(speedup_row)
        annot_matrix.append(annot_row)

    sp_array = np.array(speedup_matrix)
    annot_array = np.array(annot_matrix)

    fig, ax = plt.subplots(figsize=(11, 6), dpi=300)

    if use_log2:
        plot_data = np.log2(sp_array)
        if center_zero:
            max_abs = max(abs(plot_data.min()), abs(plot_data.max()))
            norm = mcolors.TwoSlopeNorm(vmin=-max_abs, vcenter=0.0, vmax=max_abs)
            ticks = [plot_data.min(), 0, 1, 2, 3]
            tick_labels = ["0.9x", "1.0x (Base)", "2x", "4x", "8x"]
        else:
            norm = None
            ticks = [0, 1, 2, 3]
            tick_labels = ["1x", "2x", "4x", "8x"]

        cbar_kws = {
            "label": "Speedup vs Baseline (log₂ scale)",
            "ticks": ticks,
        }
    else:
        plot_data = sp_array
        norm = None
        cbar_kws = {"label": "Speedup vs Baseline (Multiplier)"}

    heatmap = sns.heatmap(
        plot_data,
        annot=annot_array,
        fmt="",
        cmap=cmap,
        norm=norm,
        linewidths=1.0,
        linecolor="#e0e0e0",
        cbar_kws=cbar_kws,
        annot_kws={"fontsize": 11, "weight": "bold"},
        xticklabels=[f"{g}x{g}" for g in grids],
        yticklabels=stages,
        ax=ax,
    )

    if use_log2:
        cbar = heatmap.collections[0].colorbar
        cbar.set_ticklabels(tick_labels)

    title_scale = "log₂ Speedup" if use_log2 else "Speedup"
    ax.set_title(
        f"A* Optimization Performance Scaling across Grid Sizes\n(1,000 searches per cell, {title_scale} & Mean Runtime)",
        fontsize=13,
        weight="bold",
        pad=15,
    )
    ax.set_xlabel("Grid Size (Square)", fontsize=11, weight="bold", labelpad=10)
    ax.set_ylabel("Optimization Stage", fontsize=11, weight="bold")
    plt.yticks(rotation=0)
    plt.tight_layout()
    plt.savefig(output_path)
    print(f"Heatmap successfully saved to {output_path}")

def main():
    parser = argparse.ArgumentParser(description="Generate performance heatmap from results.txt")
    parser.add_argument(
        "--input",
        "-i",
        default="results.txt",
        help="Path to results.txt benchmark output (default: results.txt)",
    )
    parser.add_argument(
        "--output",
        "-o",
        default="benchmark_heatmap.png",
        help="Output image path (default: benchmark_heatmap.png)",
    )
    parser.add_argument(
        "--cmap",
        "-c",
        default="custom",
        help="Colormap to use: 'custom', 'slate_teal', or any matplotlib/seaborn colormap name (default: custom)",
    )
    parser.add_argument(
        "--linear",
        action="store_true",
        help="Use linear scale instead of log base 2 scale",
    )
    args = parser.parse_args()

    cmap_map = {
        "custom": CUSTOM_COLORS,
        "slate_teal": CUSTOM_COLORS,
        "red_cream_green": CUSTOM_COLORS,
        "rocket": "rocket_r",
    }
    selected_cmap = cmap_map.get(args.cmap, args.cmap)

    data = parse_results(args.input)
    generate_heatmap(
        data,
        output_path=args.output,
        cmap=selected_cmap,
        use_log2=not args.linear,
        center_zero=True,
    )

if __name__ == "__main__":
    main()
