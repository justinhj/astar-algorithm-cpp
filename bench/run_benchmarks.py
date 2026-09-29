#!/usr/bin/env python3
import subprocess
import sys
import re
import statistics
import os

COMMITS = [
    ("64bdb20", "Baseline (Pre-optimization)"),
    ("7424a4c", "Step 1: Open set lookup O(1)"),
    ("3ee685b", "Step 2: Intrusive min-heap (siftUp/siftDown)"),
    ("a37013f", "Step 3: Unified node map & deferred alloc"),
]

ITERATIONS = 1000
RUNS_PER_COMMIT = 5
REPO_DIR = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.join(REPO_DIR, "build")

def run_cmd(cmd, cwd=REPO_DIR):
    res = subprocess.run(cmd, shell=True, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if res.returncode != 0:
        print(f"Error running command: {cmd}\nSTDOUT:\n{res.stdout}\nSTDERR:\n{res.stderr}", file=sys.stderr)
        sys.exit(1)
    return res.stdout

def get_current_branch():
    return run_cmd("git rev-parse --abbrev-ref HEAD").strip()

def main():
    original_branch = get_current_branch()
    print(f"Starting benchmark across {len(COMMITS)} commits.")
    print(f"Searches per run: {ITERATIONS}, Repetitions: {RUNS_PER_COMMIT}\n")

    summary_results = []

    try:
        for commit_hash, desc in COMMITS:
            print("=" * 60)
            print(f"Testing commit: {commit_hash} - {desc}")
            print("=" * 60)

            # 1. Checkout commit
            run_cmd(f"git checkout {commit_hash}")

            # 2. Inject modern bench.cpp
            run_cmd("cp /tmp/new_bench.cpp bench.cpp")

            # 3. Build release binary
            print("Building bench target...")
            run_cmd("cmake -S . -B build -DCMAKE_BUILD_TYPE=Release")
            run_cmd("cmake --build build --target bench")

            bench_bin = os.path.join(BUILD_DIR, "bench")
            if not os.path.exists(bench_bin):
                print(f"Error: bench binary not found at {bench_bin}", file=sys.stderr)
                sys.exit(1)

            # 4. Run benchmark N times
            times = []
            for run_idx in range(1, RUNS_PER_COMMIT + 1):
                stdout = run_cmd(f"{bench_bin} {ITERATIONS}")
                # Parse: Total time:     X.XXXXXX seconds
                match = re.search(r"Total time:\s+([0-9.]+)\s+seconds", stdout)
                if match:
                    total_time = float(match.group(1))
                else:
                    print(f"Warning: Could not parse total time from output:\n{stdout}", file=sys.stderr)
                    sys.exit(1)

                times.append(total_time)
                print(f"  Run {run_idx}/{RUNS_PER_COMMIT}: {total_time:.6f} s")

            mean_time = statistics.mean(times)
            stdev_time = statistics.stdev(times) if len(times) > 1 else 0.0

            summary_results.append({
                "commit": commit_hash,
                "desc": desc,
                "times": times,
                "mean": mean_time,
                "stdev": stdev_time
            })
            print(f"-> Mean: {mean_time:.6f} s | Std Dev: {stdev_time:.6f} s\n")

            # Clean up bench.cpp modification before checkout
            run_cmd("git checkout -- bench.cpp")

    finally:
        print(f"Restoring git branch to '{original_branch}'...")
        run_cmd("git checkout -- bench.cpp") # Just in case it failed before this
        run_cmd(f"git checkout {original_branch}")

    # Print final summary table
    print("\n" + "=" * 75)
    print(f"{'BENCHMARK SUMMARY (1,000 searches x 5 runs)':^75}")
    print("=" * 75)
    print(f"{'Commit':<10} | {'Description':<35} | {'Mean Time':<12} | {'Std Dev':<10}")
    print("-" * 75)
    for res in summary_results:
        print(f"{res['commit']:<10} | {res['desc']:<35} | {res['mean']:.4f} s     | ±{res['stdev']:.4f} s")
    print("=" * 75)

if __name__ == "__main__":
    main()
