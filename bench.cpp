#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "stlastar.h"

// Module-level constants
const int MAP_WIDTH = 1000;
const int MAP_HEIGHT = 1000;
const unsigned int RANDOM_SEED = 12345;
const unsigned int NUM_SEARCHES = 1000000;
const int OBSTACLE_PERCENTAGE = 20;  // 20% obstacles (value 9), 80% passable terrain (value 1)

// The world map
std::vector<int> world_map;

// Map helper function
int GetMap(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return 9;
    }
    return world_map[(y * MAP_WIDTH) + x];
}

// Search node definition for the 2D grid
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

    if ((GetMap(x, y + 1) < 9) && !((parent_x == x) && (parent_y == y + 1))) {
        NewNode = MapSearchNode(x, y + 1);
        astarsearch->AddSuccessor(NewNode);
    }

    return true;
}

float MapSearchNode::GetCost(MapSearchNode& successor) {
    return static_cast<float>(GetMap(x, y));
}

int main(int argc, char* argv[]) {
    unsigned int num_searches = NUM_SEARCHES;
    if (argc > 1) {
        num_searches = static_cast<unsigned int>(std::stoul(argv[1]));
    }

    std::cout << "========================================" << std::endl;
    std::cout << "A* Search Benchmark" << std::endl;
    std::cout << "Grid size:          " << MAP_WIDTH << " x " << MAP_HEIGHT << std::endl;
    std::cout << "Random seed:        " << RANDOM_SEED << std::endl;
    std::cout << "Obstacle ratio:     " << OBSTACLE_PERCENTAGE << "%" << std::endl;
    std::cout << "Number of searches: " << num_searches << std::endl;
    std::cout << "========================================" << std::endl;

    // 1. Generate grid using fixed seed for reproducible maps
    std::mt19937 rng(RANDOM_SEED);

    world_map.resize(MAP_WIDTH * MAP_HEIGHT);
    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; ++i) {
        world_map[i] = ((rng() % 100) < static_cast<unsigned int>(OBSTACLE_PERCENTAGE)) ? 9 : 1;
    }

    std::cout << "Grid generated successfully." << std::endl;
    std::cout << "Running benchmark..." << std::endl;

    // 2. Perform searches
    // Allow one node for every grid cell, plus the separate goal node.
    AStarSearch<MapSearchNode> astarsearch(MAP_WIDTH * MAP_HEIGHT + 1);

    unsigned int successes = 0;
    unsigned int failures = 0;

    const unsigned int progress_interval = (num_searches >= 10) ? (num_searches / 10) : 1;

    auto start_time = std::chrono::steady_clock::now();

    for (unsigned int i = 0; i < num_searches; ++i) {
        MapSearchNode nodeStart;
        do {
            nodeStart.x = static_cast<int>(rng() % MAP_WIDTH);
            nodeStart.y = static_cast<int>(rng() % MAP_HEIGHT);
        } while (GetMap(nodeStart.x, nodeStart.y) >= 9);

        MapSearchNode nodeEnd;
        do {
            nodeEnd.x = static_cast<int>(rng() % MAP_WIDTH);
            nodeEnd.y = static_cast<int>(rng() % MAP_HEIGHT);
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
