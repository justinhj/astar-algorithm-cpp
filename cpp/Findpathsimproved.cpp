#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <iostream>
#include <cstring>
#include "stlastar.h" // Include the A* search library

// Define debugging constants
#define DEBUG_LISTS 0
#define DEBUG_LIST_LENGTHS_ONLY 0

// Define the dimensions of the map
const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 20;

// Create a 2D array to represent the world map
std::vector<int> world_map(MAP_WIDTH * MAP_HEIGHT, 0);

// Function to get the map value at a given (x, y) coordinate
int GetMap(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return 9; // Out of bounds, return an obstacle value
    }
    return world_map[(y * MAP_WIDTH) + x];
}

// Define the map search node class
class MapSearchNode {
public:
    int x; // x-coordinate of the node
    int y; // y-coordinate of the node

    MapSearchNode() : x(0), y(0) {}
    MapSearchNode(int px, int py) : x(px), y(py) {}

    // Heuristic function to estimate the distance to the goal
    float GoalDistanceEstimate(MapSearchNode &nodeGoal);

    // Check if this node is the goal
    bool IsGoal(MapSearchNode &nodeGoal);

    // Generate successors for this node
    bool GetSuccessors(AStarSearch<MapSearchNode> *astarsearch, MapSearchNode *parent_node);

    // Get the cost to move to a successor node
    float GetCost(MapSearchNode &successor);

    // Check if two nodes are the same state
    bool IsSameState(MapSearchNode &rhs);

    // Hash function for the node
    size_t Hash();

    // Print node information
    void PrintNodeInfo();
};

// Check if two nodes are in the same state
bool MapSearchNode::IsSameState(MapSearchNode &rhs) {
    return (x == rhs.x) && (y == rhs.y);
}

// Hash function for the node
size_t MapSearchNode::Hash() {
    size_t h1 = std::hash<int>{}(x);
    size_t h2 = std::hash<int>{}(y);
    return h1 ^ (h2 << 1);
}

// Print information about the node
void MapSearchNode::PrintNodeInfo() {
    const int strSize = 100;
    char str[strSize];
    std::snprintf(str, strSize, "Node position : (%d,%d)\n", x, y);
    std::cout << str;
}

// Heuristic function to estimate the distance to the goal
float MapSearchNode::GoalDistanceEstimate(MapSearchNode &nodeGoal) {
    return std::abs(x - nodeGoal.x) + std::abs(y - nodeGoal.y);
}

// Check if this node is the goal
bool MapSearchNode::IsGoal(MapSearchNode &nodeGoal) {
    return (x == nodeGoal.x) && (y == nodeGoal.y);
}

// Generate successors for this node
bool MapSearchNode::GetSuccessors(AStarSearch<MapSearchNode> *astarsearch, MapSearchNode *parent_node) {
    int parent_x = -1;
    int parent_y = -1;

    if (parent_node) {
        parent_x = parent_node->x;
        parent_y = parent_node->y;
    }

    MapSearchNode NewNode;

    // Generate successors by moving in four directions
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

// Get the cost to move to a successor node
float MapSearchNode::GetCost(MapSearchNode &successor) {
    return static_cast<float>(GetMap(x, y));
}

// Main function
int main(int argc, char *argv[]) {
    std::cout << "STL A* Search implementation\n(C)2001 Justin Heyes-Jones\n";

    // Create an instance of the A* search class
    AStarSearch<MapSearchNode> astarsearch;

    unsigned int SearchCount = 0;
    const unsigned int NumSearches = 1;

    while (SearchCount < NumSearches) {
        // Create a start state with random coordinates
        MapSearchNode nodeStart;
        nodeStart.x = std::rand() % MAP_WIDTH;
        nodeStart.y = std::rand() % MAP_HEIGHT;

        // Define the goal state with random coordinates
        MapSearchNode nodeEnd;
        nodeEnd.x = std::rand() % MAP_WIDTH;
        nodeEnd.y = std::rand() % MAP_HEIGHT;

        // Set the start and goal states
        astarsearch.SetStartAndGoalStates(nodeStart, nodeEnd);

        unsigned int SearchState;
        unsigned int SearchSteps = 0;

        do {
            // Perform a single step of the A* search
            SearchState = astarsearch.SearchStep();
            SearchSteps++;

#if DEBUG_LISTS
            std::cout << "Steps:" << SearchSteps << "\n";
            int len = 0;
            std::cout << "Open:\n";
            MapSearchNode *p = astarsearch.GetOpenListStart();
            while (p) {
                len++;
#if !DEBUG_LIST_LENGTHS_ONLY
                ((MapSearchNode *)p)->PrintNodeInfo();
#endif
                p = astarsearch.GetOpenListNext();
            }

            std::cout << "Open list has " << len << " nodes\n";

            len = 0;
            std::cout << "Closed:\n";
            p = astarsearch.GetClosedListStart();
            while (p) {
                len++;
#if !DEBUG_LIST_LENGTHS_ONLY
                p->PrintNodeInfo();
#endif
                p = astarsearch.GetClosedListNext();
            }
            std::cout << "Closed list has " << len << " nodes\n";
#endif

        } while (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_SEARCHING);

        if (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_SUCCEEDED) {
            std::cout << "Search found the goal state\n";

            MapSearchNode *node = astarsearch.GetSolutionStart();

#if DISPLAY_SOLUTION
            std::cout << "Displaying solution\n";
#endif
            int steps = 0;

            node->PrintNodeInfo();
            for (;;) {
                node = astarsearch.GetSolutionNext();

                if (!node) {
                    break;
                }

                node->PrintNodeInfo();
                steps++;
            }

            std::cout << "Solution steps: " << steps << std::endl;

            // Free the solution nodes when done
            astarsearch.FreeSolutionNodes();
        } else if (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_FAILED) {
            std::cout << "Search terminated. Did not find the goal state\n";
        }

        // Display the number of loops the search went through
        std::cout << "SearchSteps: " << SearchSteps << "\n";

        SearchCount++;

        astarsearch.EnsureMemoryFreed();
    }

    return 0;
}
