#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <stlastar.h>

// Module-level grid dimensions and world map
int g_grid_width = 0;
int g_grid_height = 0;
std::vector<int> world_map;

// Map helper function
int GetMap(int x, int y) {
    if (x < 0 || x >= g_grid_width || y < 0 || y >= g_grid_height) {
        return 9;
    }
    return world_map[(y * g_grid_width) + x];
}

// Search node definition for the 2D maze
class MapSearchNode {
   public:
    int x;
    int y;

    MapSearchNode() : x(0), y(0) {}
    MapSearchNode(int px, int py) : x(px), y(py) {}

    float GoalDistanceEstimate(MapSearchNode& nodeGoal);
    bool IsGoal(MapSearchNode& nodeGoal);
    bool GetSuccessors(AStarSearch<MapSearchNode>* astarsearch, MapSearchNode* parent_node);
    float GetCost(MapSearchNode& successor);
    bool IsSameState(MapSearchNode& rhs);
    size_t Hash();
};

bool MapSearchNode::IsSameState(MapSearchNode& rhs) {
    return (x == rhs.x) && (y == rhs.y);
}

size_t MapSearchNode::Hash() {
    size_t h1 = std::hash<int>{}(x);
    size_t h2 = std::hash<int>{}(y);
    return h1 ^ (h2 << 1);
}

float MapSearchNode::GoalDistanceEstimate(MapSearchNode& nodeGoal) {
    return static_cast<float>(std::abs(x - nodeGoal.x) + std::abs(y - nodeGoal.y));
}

bool MapSearchNode::IsGoal(MapSearchNode& nodeGoal) {
    return (x == nodeGoal.x) && (y == nodeGoal.y);
}

bool MapSearchNode::GetSuccessors(
    AStarSearch<MapSearchNode>* astarsearch, MapSearchNode* parent_node) {
    int parent_x = -1;
    int parent_y = -1;

    if (parent_node) {
        parent_x = parent_node->x;
        parent_y = parent_node->y;
    }

    MapSearchNode NewNode;

    // Push each possible move except backwards to the immediate parent
    if ((GetMap(x - 1, y) < 9) && !((parent_x == x - 1) && (parent_y == y))) {
        NewNode = MapSearchNode(x - 1, y);
        astarsearch->AddSuccessor(NewNode);
    }

    if ((GetMap(x, y - 1) < 9) && !((parent_x == x) && (parent_y == y - 1))) {
        NewNode = MapSearchNode(x, y - 1);
        astarsearch->AddSuccessor(NewNode);
    }

    if ((GetMap(x + 1, y) < 9) && !((parent_x == x + 1) && (parent_y == y))) {
        NewNode = MapSearchNode(x + 1, y);
        astarsearch->AddSuccessor(NewNode);
    }

    if ((GetMap(x, y + 1) < 9) && !((parent_x == x + 1) && (parent_y == y + 1))) {
        NewNode = MapSearchNode(x, y + 1);
        astarsearch->AddSuccessor(NewNode);
    }

    return true;
}

float MapSearchNode::GetCost(MapSearchNode& successor) {
    return static_cast<float>(GetMap(x, y));
}

bool LoadMazeFile(const std::string& filename, int& passable_count) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open maze file '" << filename << "'" << std::endl;
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    if (lines.empty()) {
        std::cerr << "Error: Maze file '" << filename << "' is empty." << std::endl;
        return false;
    }

    g_grid_height = static_cast<int>(lines.size());
    g_grid_width = static_cast<int>(lines[0].size());

    for (int y = 0; y < g_grid_height; ++y) {
        if (static_cast<int>(lines[y].size()) != g_grid_width) {
            std::cerr << "Error: Inconsistent line length at line " << (y + 1)
                      << " (expected " << g_grid_width << ", got " << lines[y].size() << ")" << std::endl;
            return false;
        }
    }

    world_map.resize(g_grid_width * g_grid_height);
    passable_count = 0;

    for (int y = 0; y < g_grid_height; ++y) {
        for (int x = 0; x < g_grid_width; ++x) {
            char c = lines[y][x];
            if (c == '#') {
                world_map[(y * g_grid_width) + x] = 9;
            } else {
                world_map[(y * g_grid_width) + x] = 1;
                passable_count++;
            }
        }
    }

    if (passable_count == 0) {
        std::cerr << "Error: No passable cells found in maze file '" << filename << "'." << std::endl;
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <maze_file> [iterations=1000] [seed=12345]" << std::endl;
        return 1;
    }

    std::string arg1 = argv[1];
    if (arg1 == "-h" || arg1 == "--help") {
        std::cout << "Usage: " << argv[0] << " <maze_file> [iterations=1000] [seed=12345]" << std::endl;
        return 0;
    }

    std::string maze_file = arg1;
    unsigned int num_searches = 1000;
    unsigned int seed = 12345;

    if (argc > 2) {
        num_searches = static_cast<unsigned int>(std::stoul(argv[2]));
    }
    if (argc > 3) {
        seed = static_cast<unsigned int>(std::stoul(argv[3]));
    }

    if (num_searches < 1) num_searches = 1;

    int passable_count = 0;
    if (!LoadMazeFile(maze_file, passable_count)) {
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "A* Maze Search Benchmark" << std::endl;
    std::cout << "Maze file:          " << maze_file << std::endl;
    std::cout << "Grid size:          " << g_grid_width << " x " << g_grid_height << std::endl;
    std::cout << "Passable cells:     " << passable_count << std::endl;
    std::cout << "Random seed:        " << seed << std::endl;
    std::cout << "Number of searches: " << num_searches << std::endl;
    std::cout << "========================================" << std::endl;

    std::cout << "Running benchmark..." << std::endl;

    std::mt19937 rng(seed);
    AStarSearch<MapSearchNode> astarsearch((g_grid_width * g_grid_height) + 1);

    unsigned int successes = 0;
    unsigned int failures = 0;

    const unsigned int progress_interval = (num_searches >= 10) ? (num_searches / 10) : 1;

    auto start_time = std::chrono::steady_clock::now();

    for (unsigned int i = 0; i < num_searches; ++i) {
        MapSearchNode nodeStart;
        do {
            nodeStart.x = static_cast<int>(rng() % g_grid_width);
            nodeStart.y = static_cast<int>(rng() % g_grid_height);
        } while (GetMap(nodeStart.x, nodeStart.y) >= 9);

        MapSearchNode nodeEnd;
        do {
            nodeEnd.x = static_cast<int>(rng() % g_grid_width);
            nodeEnd.y = static_cast<int>(rng() % g_grid_height);
        } while (GetMap(nodeEnd.x, nodeEnd.y) >= 9);

        astarsearch.SetStartAndGoalStates(nodeStart, nodeEnd);

        unsigned int SearchState;
        do {
            SearchState = astarsearch.SearchStep();
        } while (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_SEARCHING);

        if (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_SUCCEEDED) {
            successes++;
            astarsearch.FreeSolutionNodes();
        } else {
            failures++;
        }

        astarsearch.EnsureMemoryFreed();

        if (progress_interval > 0 && (i + 1) % progress_interval == 0) {
            unsigned int pct =
                static_cast<unsigned int>((static_cast<uint64_t>(i + 1) * 100) / num_searches);
            std::cout << "  Progress: " << (i + 1) << " / " << num_searches << " (" << pct
                      << "%)..." << std::endl;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;

    double total_sec = elapsed_seconds.count();
    double avg_sec = total_sec / static_cast<double>(num_searches);
    double avg_microsec = avg_sec * 1e6;
    double avg_nanosec = avg_sec * 1e9;

    std::cout << "\n----------------------------------------" << std::endl;
    std::cout << "Benchmark Results:" << std::endl;
    std::cout << "Total searches: " << num_searches << std::endl;
    std::cout << "  Succeeded:    " << successes << std::endl;
    std::cout << "  Failed:       " << failures << std::endl;
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Total time:     " << total_sec << " seconds (" << (total_sec * 1000.0) << " ms)"
              << std::endl;
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Average time:   " << avg_microsec << " us (" << avg_nanosec << " ns, "
              << std::setprecision(8) << avg_sec << " s) per search" << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    return 0;
}
