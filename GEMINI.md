# A* Algorithm C++ Implementation

## Project Overview

This project provides a clean, efficient, and header-only C++ implementation of the A* (A-Star) pathfinding algorithm. It is designed for high-performance applications like video games and includes an optional fixed-size memory allocator (`fsa.h`) to minimize memory fragmentation and allocation overhead.

The core logic resides in `stlastar.h`, which uses C++ templates to work with any user-defined state class that satisfies the required interface.

## Building and Running

### Prerequisites
*   C++ compiler supporting C++11 (e.g., `g++`, `clang++`).
*   `make` utility.

### Build Commands
The project uses a `makefile` to manage builds.

*   **Build All:**
    ```bash
    make
    ```
    This compiles the library examples and tests, producing the following executables:
    *   `8puzzle`: Solves the 8-puzzle sliding tile game.
    *   `findpath`: Finds a path on a simple grid map.
    *   `minpathbucharest`: Solves the "classic" AI problem of finding the shortest path to Bucharest.
    *   `tests`: Runs the unit tests.

*   **Run Tests:**
    ```bash
    make test
    ```

*   **Clean Build:**
    ```bash
    make clean
    ```

### Running Examples
*   **8-Puzzle:**
    ```bash
    ./8puzzle [board_string]
    # Example: ./8puzzle 013824765
    ```
*   **Pathfinder:**
    ```bash
    ./findpath
    ```

## Development Conventions

### Core Architecture
*   **`stlastar.h`**: The main header file containing the `AStarSearch` class template. This is a header-only library; simply include this file in your project.
*   **`fsa.h`**: A fixed-size block memory allocator used internally by `AStarSearch` to optimize node allocation. Can be toggled via `USE_FSA_MEMORY` in `stlastar.h`.

### User State Interface
To use the A* search, you must define a class (e.g., `MapSearchNode`) that represents a state in your search space. This class acts as a template argument to `AStarSearch` and **must** implement the following methods:

```cpp
class YourStateClass {
public:
    // Heuristic estimate of distance to goal (e.g., Manhattan distance, Euclidean distance)
    float GoalDistanceEstimate(YourStateClass &nodeGoal);

    // Returns true if this node is the goal
    bool IsGoal(YourStateClass &nodeGoal);

    // Generates successors and adds them to the search
    // Implementation should call astarsearch->AddSuccessor(NewNode) for each valid move
    bool GetSuccessors(AStarSearch<YourStateClass> *astarsearch, YourStateClass *parent_node);

    // Cost of moving from this node to the successor
    float GetCost(YourStateClass &successor);

    // Returns true if this state is identical to rhs
    bool IsSameState(YourStateClass &rhs);

    // Returns a hash of the state (required for std::unordered_set)
    size_t Hash();
};
```

### Usage Pattern
1.  Instantiate `AStarSearch<YourStateClass> astarsearch;`.
2.  Create start and goal states.
3.  Call `astarsearch.SetStartAndGoalStates(start, goal)`.
4.  Loop `astarsearch.SearchStep()` until the state is `SEARCH_STATE_SUCCEEDED` or `SEARCH_STATE_FAILED`.
5.  Retrieve the path using `astarsearch.GetSolutionStart()`, `GetSolutionNext()`, etc.
6.  Call `astarsearch.FreeSolutionNodes()` and `astarsearch.EnsureMemoryFreed()` to clean up.

### Testing
*   Tests are located in `tests.cpp`.
*   Ensure all tests pass with `make test` before submitting changes.
