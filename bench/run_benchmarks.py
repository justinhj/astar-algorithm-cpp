#!/usr/bin/env python3
import argparse
import subprocess
import sys
import re
import statistics
import os

BENCHMARKS = [
    ("bench_baseline", "64bdb20", "Baseline (Pre-optimization)"),
    ("bench_step1", "7424a4c", "Step 1: Open set lookup O(1)"),
    ("bench_step2", "3ee685b", "Step 2: Intrusive min-heap (siftUp/siftDown)"),
    ("bench_step3", "a37013f", "Step 3: Unified node map & deferred alloc"),
    ("bench", "master", "Current master"),
]

REPO_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD_DIR = os.path.join(REPO_DIR, "build")

def run_cmd(cmd, cwd=REPO_DIR):
    res = subprocess.run(cmd, shell=True, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        print(f"Error running command: {cmd}\nSTDOUT:\n{res.stdout}\nSTDERR:\n{res.stderr}", file=sys.stderr)
        sys.exit(1)
    return res.stdout

def parse_args():
    parser = argparse.ArgumentParser(
        description="A* Optimization Benchmark Suite across historical stages and current master."
    )
    parser.add_argument(
        "-g", "--grid-size",
        type=int,
        default=64,
        help="Width and height of the square map (default: 64)"
    )
    parser.add_argument(
        "-i", "--iterations",
        type=int,
        default=1000,
        help="Number of searches per benchmark run (default: 1000)"
    )
    parser.add_argument(
        "-s", "--seed",
        type=int,
        default=12345,
        help="Random seed for map and coordinates (default: 12345)"
    )
    parser.add_argument(
        "-r", "--runs",
        type=int,
        default=5,
        help="Repetitions per stage to compute mean and std dev (default: 5)"
    )

    args = parser.parse_args()
    return args.grid_size, args.iterations, args.seed, args.runs

def main():
    grid_size, iterations, seed, runs = parse_args()

    print(f"============================================================")
    print(f"A* Optimization Benchmark Suite")
    print(f"Parameters: grid={grid_size}x{grid_size}, searches={iterations}, seed={seed}, repetitions={runs}")
    print(f"============================================================\n")

    # 1. Build all benchmark targets
    print("Building all benchmark targets...")
    if not os.path.exists(os.path.join(BUILD_DIR, "CMakeCache.txt")):
        run_cmd("cmake -S . -B build -DCMAKE_BUILD_TYPE=Release")
    run_cmd("cmake --build build --target bench_all")
    print("Build complete.\n")

    summary_results = []

    # 2. Run each target
    for target_name, commit_hash, desc in BENCHMARKS:
        print("=" * 60)
        print(f"Testing {target_name} ({commit_hash}) - {desc}")
        print("=" * 60)

        bench_bin = os.path.join(BUILD_DIR, target_name)
        if not os.path.exists(bench_bin):
            print(f"Error: binary not found at {bench_bin}", file=sys.stderr)
            sys.exit(1)

        times = []
        for run_idx in range(1, runs + 1):
            cmd = f"{bench_bin} {grid_size} {iterations} {seed}"
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

        summary_results.append({
            "target": target_name,
            "commit": commit_hash,
            "desc": desc,
            "times": times,
            "mean": mean_time,
            "stdev": stdev_time
        })
        print(f"-> Mean: {mean_time:.6f} s | Std Dev: {stdev_time:.6f} s\n")

    # 3. Print final summary table
    print("\n" + "=" * 90)
    title = f"BENCHMARK SUMMARY ({iterations:,} searches x {runs} runs, grid={grid_size}x{grid_size})"
    print(f"{title:^90}")
    print("=" * 90)
    print(f"{'Target':<16} | {'Commit':<8} | {'Description':<40} | {'Mean Time':<11} | {'Std Dev':<9}")
    print("-" * 90)
    baseline_mean = summary_results[0]["mean"] if summary_results else None
    for res in summary_results:
        speedup = f"({baseline_mean / res['mean']:.1f}x)" if baseline_mean and res['mean'] > 0 else ""
        print(f"{res['target']:<16} | {res['commit']:<8} | {res['desc']:<40} | {res['mean']:.4f} s    | ±{res['stdev']:.4f} s {speedup}")
    print("=" * 90)

if __name__ == "__main__":
    main()
