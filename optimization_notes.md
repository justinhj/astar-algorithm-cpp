# Benchmark and Optimization Notes

## Summary

The benefit of the A* optimizations depends on the search workload. On the
1,000 x 1,000 grid with 20% obstacles and enough node capacity to finish
searches, the current version was about 19 times faster than the original
benchmark commit in a 500-search comparison. The last optimization commit
alone was about 1.55 times faster than the stable version before it.

Obstacle density changes both the amount of search work and whether a path
exists. On a smaller grid, searches became most expensive around 35-40%
obstacles, then became cheap at 50% because almost all start and goal pairs
were in separate, small connected regions. This means a speedup measured at
the benchmark's fixed 20% obstacle ratio does not describe every map density.

## The original node limit

The benchmark originally constructed `AStarSearch` with its default fixed
allocator capacity of 1,000 nodes. That is too small for many searches on
the 1,000 x 1,000 grid. When `AddSuccessor` could not allocate a node, it
returned `false`, but `bench.cpp` ignored that return value. The resulting
search was reported as an ordinary failure rather than an allocation failure.

On the current optimization code, a 1,000-search sample at 20% obstacles with
the old capacity produced 16 successful searches and 984 reported failures.
Instrumentation found rejected successors in 981 of those failed searches.
With the larger capacity, the same sample produced 997 successful searches
and three searches with no path.
The old timings therefore measured mostly searches cut short by the allocator.

`bench.cpp` now gives the allocator `MAP_WIDTH * MAP_HEIGHT + 1` slots:
one per grid cell and one for the separate goal node. This is 1,000,001 slots
for the default map. The larger pool also makes the default million-search
benchmark much longer to run.

## Optimization stages on the full-size grid

Each stage used the same 1,000 x 1,000 grid, seed `12345`, 20% obstacles,
500 searches, and a 1,000,001-node pool. Each historical revision was built
from its own `bench.cpp`, `stlastar.h`, and `fsa.h`, with only the benchmark's
allocator capacity changed. Builds used Apple Clang 21 with
`-std=c++11 -O3 -DNDEBUG`. The table shows one run per stage, using the
benchmark's timed search loop.

| Stage | Commit | Total time | Time/search | Speedup vs. baseline |
| --- | --- | ---: | ---: | ---: |
| Benchmark baseline | `30997b4` | 105.435 s | 210.869 ms | 1.00x |
| Open-list lookup | `7424a4c` | 40.335 s | 80.670 ms | 2.61x |
| Indexed heap | `3ee685b` | 8.219 s | 16.438 ms | 12.83x |
| Stable pre-final version | `76556a6` | 8.466 s | 16.932 ms | 12.45x |
| Current optimization | `a37013f` | 5.469 s | 10.938 ms | 19.28x |

Every stage found 499 paths and reported one no-path search. The large
improvement from the open-list stage to the indexed-heap stage is consistent
with replacing expensive heap rebuilding with indexed updates. The current
optimization was about 35% faster than the stable pre-final version. The
roughly 3% difference between the indexed-heap and pre-final rows is too
small to interpret confidently from one run each.

A 20,000-search comparison was stopped before it produced stage results.
The measurements above are from the subsequent 500-search comparison.

## Effect of obstacle density

To examine density separately, each stage was also run on a 100 x 100 grid
with a 12,000-node pool, 1,000 searches, and the same fixed seed. A temporary
benchmark variant allowed the obstacle ratio to be changed and counted
rejected successor allocations; none occurred. These values are median
microseconds per search from three runs at each density, with the same
compiler settings as above.

| Obstacles | Paths found | Baseline | Open-list lookup | Indexed heap | Pre-final | Current |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 0% | 1,000 | 283.40 | 177.10 | 104.91 | 105.34 | 58.56 |
| 10% | 1,000 | 175.45 | 128.01 | 83.80 | 86.00 | 48.65 |
| 20% | 994 | 134.62 | 111.75 | 93.15 | 96.80 | 55.19 |
| 30% | 940 | 135.81 | 134.07 | 133.13 | 136.62 | 85.86 |
| 35% | 855 | 188.27 | 194.61 | 201.24 | 205.34 | 132.16 |
| 40% | 595 | 198.59 | 222.55 | 231.58 | 235.30 | 153.92 |
| 50% | 6 | 3.90 | 5.05 | 5.13 | 5.35 | 2.95 |
| 60% | 4 | 0.82 | 1.17 | 1.20 | 1.11 | 0.77 |

At low density, most pairs are connected. Obstacles can remove alternative
moves, but they can also force detours. Around 35-40% obstacles in this
sample, searches often explored substantial regions before finding a path
or exhausting the reachable region. At 50-60%, most searches were short
failures in small disconnected regions, so the average time fell sharply.

Density also changes the relative value of data structures. For example,
the open-list lookup change was about 38% faster than the baseline at 0%
obstacles, but about 12% slower at 40%. Its hash-table maintenance costs
more when the frontier is small. The current version had lower median times
at every tested density, although the difference at 60% is very small.

## How to interpret these results

- The full-size stage results cover only 20% obstacles. The density sweep
  used a smaller grid, so its timings and speedups cannot be substituted for
  full-size results at those ratios.
- Across different densities, the benchmark's rejection sampling changes
  the start and goal sequence. Density also changes the proportion of
  successful and failed searches. The averages therefore compare different
  mixes of search tasks, not the same endpoint pairs on altered maps.
- Changing the heap also changes tie ordering and the number of expanded
  nodes. These timings measure the whole search, not only the cost per node
  of an individual optimization.
- The full-size stage table has one run per commit. Large differences are
  clear, while small differences need repeated runs before drawing a firm
  conclusion. At 50-60% obstacles on the small grid, each run took only a
  few milliseconds, so small timing differences are especially sensitive
  to measurement noise.
