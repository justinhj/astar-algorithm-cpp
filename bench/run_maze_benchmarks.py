#!/usr/bin/env python3
import argparse
import os
import re
import statistics
import subprocess
import sys

BENCHMARKS = [
    ("mazebench_baseline", "64bdb20", "Baseline (Pre-optimization)"),
    ("mazebench_step1", "7424a4c", "Step 1: Open set lookup O(1)"),
    ("mazebench_step2", "3ee685b", "Step 2: Intrusive min-heap (siftUp/siftDown)"),
    ("mazebench_step3", "a37013f", "Step 3: Unified node map & deferred alloc"),
    ("mazebench", "master", "Current master"),
]

REPO_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(REPO_DIR, "build")
MAZES_DIR = os.path.join(REPO_DIR, "bench", "mazes")
MAZEGEN_SCRIPT = os.path.join(REPO_DIR, "bench", "mazegen.py")

def run_cmd(cmd, cwd=REPO_DIR):
    res = subprocess.run(cmd, shell=True, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        print(f"Error running command: {cmd}\nSTDOUT:\n{res.stdout}\nSTDERR:\n{res.stderr}", file=sys.stderr)
        sys.exit(1)
    return res.stdout

def parse_args():
    parser = argparse.ArgumentParser(
        description="A* Maze Optimization Benchmark Suite across historical stages and current master."
    )
    parser.add_argument(
        "-s", "--sizes",
        type=str,
        default="16,32,64,128,256",
        help="Comma-separated cell sizes for maze generation (default: '16,32,64,128,256')"
    )
    parser.add_argument(
        "-a", "--algo",
        type=str,
        default="prim",
        choices=["prim", "dfs"],
        help="Maze generation algorithm (default: 'prim')"
    )
    parser.add_argument(
        "-b", "--braid",
        type=float,
        default=0.3,
        help="Braid rate [0.0 to 1.0] for loops (default: 0.3)"
    )
    parser.add_argument(
        "-i", "--iterations",
        type=int,
        default=1000,
        help="Number of searches per benchmark run (default: 1000)"
    )
    parser.add_argument(
        "--seed",
        type=int,
        default=12345,
        help="Random seed for maze and coordinates (default: 12345)"
    )
    parser.add_argument(
        "-r", "--runs",
        type=int,
        default=3,
        help="Repetitions per stage to compute mean and std dev (default: 3)"
    )
    parser.add_argument(
        "--single",
        action="store_true",
        help="Only run the current master target (mazebench)"
    )
    parser.add_argument(
        "-o", "--output",
        type=str,
        default="results_maze.txt",
        help="Path to save benchmark text results (default: results_maze.txt)"
    )
    parser.add_argument(
        "--heatmap",
        type=str,
        default="maze_benchmark_heatmap.png",
        help="Path to save output heatmap image (default: maze_benchmark_heatmap.png, set to empty to disable)"
    )
    parser.add_argument(
        "--mazes-dir",
        type=str,
        default=MAZES_DIR,
        help=f"Directory to store and load maze files (default: {MAZES_DIR})"
    )
    parser.add_argument(
        "--force-generate",
        action="store_true",
        help="Force regeneration of maze files even if they already exist"
    )

    args = parser.parse_args()
    try:
        sizes = [int(x.strip()) for x in args.sizes.split(",") if x.strip()]
    except ValueError:
        parser.error(f"Invalid sizes specification: '{args.sizes}'. Expected comma-separated integers.")

    if not sizes:
        parser.error("At least one size must be specified.")

    return sizes, args.algo, args.braid, args.iterations, args.seed, args.runs, args.single, args.output, args.heatmap, args.mazes_dir, args.force_generate

def ensure_mazegen():
    if not os.path.exists(MAZEGEN_SCRIPT):
        print(f"mazegen.py not found at {MAZEGEN_SCRIPT}, configuring CMake to fetch it...")
        run_cmd("cmake -S . -B build -DCMAKE_BUILD_TYPE=Release")
    if not os.path.exists(MAZEGEN_SCRIPT):
        print(f"Error: Unable to find or fetch mazegen.py at {MAZEGEN_SCRIPT}", file=sys.stderr)
        sys.exit(1)

def generate_maze(size, seed, algo, braid, mazes_dir=MAZES_DIR, force_generate=False):
    os.makedirs(mazes_dir, exist_ok=True)
    maze_file = os.path.join(mazes_dir, f"maze_s{size}_seed{seed}_{algo}_b{braid:.2f}.txt")
    if os.path.exists(maze_file) and not force_generate:
        return maze_file
    cmd = f"{sys.executable} \"{MAZEGEN_SCRIPT}\" {size} {seed} --algo {algo} --braid {braid}"
    stdout = run_cmd(cmd)
    with open(maze_file, "w") as f:
        f.write(stdout)
    return maze_file

def main():
    sizes, algo, braid, iterations, seed, runs, single, output_file, heatmap_file, mazes_dir, force_generate = parse_args()

    targets_to_run = [b for b in BENCHMARKS if b[0] == "mazebench"] if single else BENCHMARKS

    print("============================================================")
    print("A* Maze Benchmark Suite")
    rendered_dims = [f"{2 * s + 1}x{2 * s + 1}" for s in sizes]
    print(f"Parameters: cell sizes={sizes} (grids={rendered_dims}), algo={algo}, braid={braid}")
    print(f"Searches={iterations}, seed={seed}, repetitions={runs}")
    print(f"Mazes directory: {mazes_dir}")
    print("============================================================\n")

    # 1. Ensure mazegen script and build benchmark targets
    ensure_mazegen()

    print("Building benchmark targets...")
    if not os.path.exists(os.path.join(BUILD_DIR, "CMakeCache.txt")):
        run_cmd("cmake -S . -B build -DCMAKE_BUILD_TYPE=Release")

    if single:
        run_cmd("cmake --build build --target mazebench")
    else:
        run_cmd("cmake --build build --target mazebench_all")
    print("Build complete.\n")

    all_size_results = {}
    log_lines = []

    for size in sizes:
        rendered_dim = 2 * size + 1
        log_lines.append(f"\n======================================================================")
        log_lines.append(f"BENCHMARK RUN: MAZE_SIZE = {rendered_dim} (CELLS = {size})")
        log_lines.append(f"======================================================================")
        log_lines.append(f"Parameters: grid={rendered_dim}x{rendered_dim}, searches={iterations}, seed={seed}, repetitions={runs}\n")

        print("*" * 70)
        print(f"Generating and benchmarking maze: cell size {size} ({rendered_dim}x{rendered_dim} grid)")
        print("*" * 70)

        maze_file = generate_maze(size, seed, algo, braid, mazes_dir, force_generate)
        print(f"Maze file: {maze_file}\n")

        size_summary = []

        for target_name, commit_hash, desc in targets_to_run:
            print("=" * 60)
            print(f"Testing {target_name} ({commit_hash}) - {desc}")
            print("=" * 60)

            bench_bin = os.path.join(BUILD_DIR, target_name)
            if not os.path.exists(bench_bin):
                print(f"Error: binary not found at {bench_bin}", file=sys.stderr)
                sys.exit(1)

            times = []
            for run_idx in range(1, runs + 1):
                cmd = f"\"{bench_bin}\" \"{maze_file}\" {iterations} {seed}"
                stdout = run_cmd(cmd)

                match = re.search(r"Total time:\s+([0-9.]+)\s+seconds", stdout)
                if match:
                    total_time = float(match.group(1))
                else:
                    print(f"Warning: Could not parse total time from output:\n{stdout}", file=sys.stderr)
                    sys.exit(1)

                times.append(total_time)
                print(f"  Run {run_idx}/{runs}: {total_time:.6f} s")

            mean_time = statistics.mean(times)
            stdev_time = statistics.stdev(times) if len(times) > 1 else 0.0

            size_summary.append({
                "target": target_name,
                "commit": commit_hash,
                "desc": desc,
                "times": times,
                "mean": mean_time,
                "stdev": stdev_time,
            })
            print(f"-> Mean: {mean_time:.6f} s | Std Dev: {stdev_time:.6f} s\n")

        all_size_results[size] = size_summary

        # Add section summary to log lines
        log_lines.append("-" * 90)
        baseline_mean = size_summary[0]["mean"] if size_summary else None
        for res in size_summary:
            speedup = f"({baseline_mean / res['mean']:.1f}x)" if baseline_mean and res['mean'] > 0 else ""
            log_lines.append(f"{res['target']:<20} | {res['commit']:<8} | {res['desc']:<36} | {res['mean']:.4f} s    | ±{res['stdev']:.4f} s {speedup}")
        log_lines.append("-" * 90)

    # Print summary tables
    for size, summary_results in all_size_results.items():
        rendered_dim = 2 * size + 1
        print("\n" + "=" * 90)
        title = (
            f"MAZE BENCHMARK SUMMARY: size {size} ({rendered_dim}x{rendered_dim} grid, "
            f"{algo}, braid {braid:.2f}, {iterations:,} searches x {runs} runs)"
        )
        print(f"{title:^90}")
        print("=" * 90)
        print(f"{'Target':<20} | {'Commit':<8} | {'Description':<36} | {'Mean Time':<11} | {'Std Dev':<9}")
        print("-" * 90)
        baseline_mean = summary_results[0]["mean"] if summary_results else None
        for res in summary_results:
            speedup = f"({baseline_mean / res['mean']:.1f}x)" if baseline_mean and res['mean'] > 0 else ""
            print(f"{res['target']:<20} | {res['commit']:<8} | {res['desc']:<36} | {res['mean']:.4f} s    | ±{res['stdev']:.4f} s {speedup}")
        print("=" * 90)

    # Save output to text file if specified
    if output_file:
        out_path = os.path.join(REPO_DIR, output_file) if not os.path.isabs(output_file) else output_file
        with open(out_path, "w") as f:
            f.write("\n".join(log_lines) + "\n")
        print(f"\nSaved benchmark results to {out_path}")

    # Generate heatmap if requested
    if heatmap_file:
        try:
            if REPO_DIR not in sys.path:
                sys.path.insert(0, REPO_DIR)
            import bench.plot_maze_heatmap as pmh
            heat_path = os.path.join(REPO_DIR, heatmap_file) if not os.path.isabs(heatmap_file) else heatmap_file
            data = pmh.parse_results(out_path if output_file else "results_maze.txt")
            pmh.generate_heatmap(data, output_path=heat_path)
            print(f"Generated heatmap at {heat_path}")
        except Exception as e:
            print(f"Warning: Failed to generate heatmap: {e}", file=sys.stderr)

if __name__ == "__main__":
    main()
