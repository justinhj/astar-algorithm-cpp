# A* Algorithm Optimizations

This document summarizes the performance optimizations applied to the A* search implementation in `stlastar.h` and `fsa.h`. These changes significantly reduce memory overhead, hash table lookups, and allocation cycles.

## Performance Impact

On the 100,000-search grid benchmark (`bench.cpp`):
- **Baseline (Before):** 22.58 seconds (~225.8 μs/search)
- **Optimized (After):** 13.48 seconds (~134.8 μs/search)

This represents a **~40% performance improvement** overall.

## Applied Optimizations

### 1. Merged Open and Closed Data Structures
**Previously:** The search frontier was maintained using three separate data structures: `m_OpenList` (a vector used as a min-heap), `m_OpenSet` (an `unordered_set` for O(1) membership testing in the open list), and `m_ClosedList` (an `unordered_set` for closed list testing).
**Optimization:** `m_OpenSet` and `m_ClosedList` were merged into a single `std::unordered_set<Node*>` called `m_NodeMap`. Open vs. closed state is now determined entirely by checking the intrusive `heap_index` of the node. If `heap_index == SIZE_MAX`, the node is closed; otherwise, it is open. This cuts hash insertions, lookups, and memory overhead in half.

### 2. Deferred Node Allocation (Inline Successor Processing)
**Previously:** When expanding a node, `GetSuccessors` would call `AddSuccessor`, which proactively called `AllocateNode()` to create a full node on the heap (or FSA pool) and added it to an `m_Successors` vector. Most of these (e.g., 3 out of 4 on a grid map) were duplicates that had to be immediately freed.
**Optimization:** `m_Successors` was removed entirely. `AddSuccessor` now processes successors inline using the current expanding node (`m_CurrentExpandingNode`). It performs the cost calculation and `m_NodeMap` duplicate check using a cheap stack-allocated dummy node. `AllocateNode()` is only invoked if the successor is genuinely new and needs to be placed on the open list.

### 3. Skipped Closed-List Re-opening
**Previously:** Every successor expansion checked if the node existed in the closed list and unconditionally compared costs to see if it needed to be reopened and moved back to the open list.
**Optimization:** A template parameter `ConsistentHeuristic` was added (`template <class UserState, bool ConsistentHeuristic = true>`). Because most A* implementations (using distances like Manhattan or Euclidean) are consistent/monotone, nodes on the closed list can never be improved. When `ConsistentHeuristic` is `true`, the closed-list cost comparison and reopening logic is completely skipped as dead code.

### 4. Simplified FSA Allocator (`fsa.h`)
**Previously:** `FixedSizeAllocator` used a doubly-linked list with `pPrev`, `pNext`, and a boolean `bAllocated` flag to maintain a separate "used list" alongside the free list.
**Optimization:** The "used list" tracking was completely removed. `fsa.h` now implements a much leaner, singly-linked free list, drastically reducing the number of pointer assignments per `alloc()` and `free()`, and improving cache locality.

### 5. Removed Inefficient Float Hashing
**Previously:** `bench.cpp` and `findpath.cpp` utilized `std::hash<float>` and casted their integer grid coordinates to floats before hashing, which introduced an unnecessary conversion penalty.
**Optimization:** `Hash()` functions were updated to use `std::hash<int>` directly on their integer coordinates.

### 6. Pre-allocated Vector Capacity
**Optimization:** Added `m_OpenList.reserve(...)` in `SetStartAndGoalStates` to avoid small micro-allocations/resizes at the very beginning of the search expansion.

---

## Further Opportunities (Pending)
*   **Const-Correctness:** Updating the `UserState` interface to enforce `const` on `IsSameState`, `Hash`, `GetCost`, and `GoalDistanceEstimate`. This is highly recommended for C++ best practices and enables further compiler optimizations, but it requires breaking API changes to user-provided state classes.
*   **Growable FSA Pool:** The FSA pool could be updated to a chunked allocator that adds new blocks dynamically instead of throwing an out-of-memory error when `MaxNodes` is exhausted.
