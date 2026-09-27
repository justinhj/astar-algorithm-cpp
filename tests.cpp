// A unit test suite

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "stlastar.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

using std::cout;
using std::hash;

const int MAP_WIDTH = 20;
const int MAP_HEIGHT = 20;

int world_map[MAP_WIDTH * MAP_HEIGHT] = {

    // 0001020304050607080910111213141516171819
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 00
    1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 1,  // 01
    1, 9, 9, 1, 1, 9, 9, 9, 1, 9, 1, 9, 1, 9, 1, 9, 9, 9, 1, 1,  // 02
    1, 9, 9, 1, 1, 9, 9, 9, 1, 9, 1, 9, 1, 9, 1, 9, 9, 9, 1, 1,  // 03
    1, 9, 1, 9, 1, 9, 9, 9, 1, 9, 1, 9, 1, 1, 1, 1, 9, 9, 1, 1,  // 04
    1, 9, 1, 1, 9, 1, 1, 1, 1, 9, 1, 1, 1, 1, 9, 1, 1, 1, 1, 1,  // 05
    1, 9, 9, 9, 9, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 1, 1, 1, 1, 1,  // 06
    1, 9, 9, 9, 9, 9, 9, 9, 9, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 1,  // 07
    1, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 1, 1, 1, 1, 1, 1, 1, 1,  // 08
    1, 9, 1, 9, 9, 9, 9, 9, 9, 9, 1, 1, 9, 9, 9, 9, 9, 9, 9, 1,  // 09
    1, 9, 1, 1, 1, 1, 9, 1, 1, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 10
    1, 9, 9, 9, 9, 9, 1, 9, 1, 9, 1, 9, 9, 9, 9, 9, 1, 1, 1, 1,  // 11
    1, 9, 1, 9, 1, 9, 9, 9, 1, 9, 1, 9, 1, 9, 1, 9, 9, 9, 1, 1,  // 12
    1, 9, 1, 9, 1, 9, 9, 9, 1, 9, 1, 9, 1, 9, 1, 9, 9, 9, 1, 1,  // 13
    1, 9, 1, 1, 1, 1, 9, 9, 1, 9, 1, 9, 1, 1, 1, 1, 9, 9, 1, 1,  // 14
    1, 9, 1, 1, 9, 1, 1, 1, 1, 9, 1, 1, 1, 1, 9, 1, 1, 1, 1, 1,  // 15
    1, 9, 9, 9, 9, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 1, 1, 1, 1, 1,  // 16
    1, 1, 9, 9, 9, 9, 9, 9, 9, 1, 1, 1, 9, 9, 9, 1, 9, 9, 9, 9,  // 17
    1, 9, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 1, 1, 1, 1, 1, 1, 1, 1,  // 18
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 19

};

// map helper functions

int GetMap(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return 9;
    }

    return world_map[(y * MAP_WIDTH) + x];
}

// Definitions

class MapSearchNode {
   public:
    int x;  // the (x,y) positions of the node
    int y;

    MapSearchNode() {
        x = y = 0;
    }
    MapSearchNode(int px, int py) {
        x = px;
        y = py;
    }

    float GoalDistanceEstimate(MapSearchNode& nodeGoal);
    bool IsGoal(MapSearchNode& nodeGoal);
    bool GetSuccessors(AStarSearch<MapSearchNode>* astarsearch, MapSearchNode* parent_node);
    float GetCost(MapSearchNode& successor);
    bool IsSameState(MapSearchNode& rhs);
    size_t Hash();

    void PrintNodeInfo();
};

bool MapSearchNode::IsSameState(MapSearchNode& rhs) {
    // same state in a maze search is simply when (x,y) are the same
    if ((x == rhs.x) && (y == rhs.y)) {
        return true;
    } else {
        return false;
    }
}

void MapSearchNode::PrintNodeInfo() {
    const int strSize = 100;
    char str[strSize];
    snprintf(str, strSize, "Node position : (%d,%d)\n", x, y);

    cout << str;
}

// Here's the heuristic function that estimates the distance from a Node
// to the Goal.

float MapSearchNode::GoalDistanceEstimate(MapSearchNode& nodeGoal) {
    return abs(x - nodeGoal.x) + abs(y - nodeGoal.y);
}

bool MapSearchNode::IsGoal(MapSearchNode& nodeGoal) {
    if ((x == nodeGoal.x) && (y == nodeGoal.y)) {
        return true;
    }

    return false;
}

// This generates the successors to the given Node. It uses a helper function called
// AddSuccessor to give the successors to the AStar class. The A* specific initialisation
// is done for each node internally, so here you just set the state information that
// is specific to the application
bool MapSearchNode::GetSuccessors(
    AStarSearch<MapSearchNode>* astarsearch, MapSearchNode* parent_node) {
    int parent_x = -1;
    int parent_y = -1;

    if (parent_node) {
        parent_x = parent_node->x;
        parent_y = parent_node->y;
    }

    MapSearchNode NewNode;

    // push each possible move except allowing the search to go backwards

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

// given this node, what does it cost to move to successor. In the case
// of our map the answer is the map terrain value at this node since that is
// conceptually where we're moving

float MapSearchNode::GetCost(MapSearchNode& successor) {
    return (float)GetMap(x, y);
}

size_t MapSearchNode::Hash() {
    size_t h1 = hash<float>{}(x);
    size_t h2 = hash<float>{}(y);
    return h1 ^ (h2 << 1);
}

TEST_CASE("Map Search") {
    AStarSearch<MapSearchNode> astarsearch;

    unsigned int SearchCount = 0;

    const unsigned int NumSearches = 1;

    while (SearchCount < NumSearches) {
        // Create a start state
        MapSearchNode nodeStart;
        nodeStart.x = 0;
        nodeStart.y = 0;

        // Define the goal state
        MapSearchNode nodeEnd;
        nodeEnd.x = 3;
        nodeEnd.y = 3;

        // Set Start and goal states

        astarsearch.SetStartAndGoalStates(nodeStart, nodeEnd);

        unsigned int SearchState;
        unsigned int SearchSteps = 0;

        do {
            SearchState = astarsearch.SearchStep();

            SearchSteps++;
        } while (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_SEARCHING);

        if (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_SUCCEEDED) {
            MapSearchNode* node = astarsearch.GetSolutionStart();

            for (;;) {
                node = astarsearch.GetSolutionNext();

                if (!node) {
                    break;
                }
            };

            // Once you're done with the solution you can free the nodes up
            astarsearch.FreeSolutionNodes();

        }

        SearchCount++;

        astarsearch.EnsureMemoryFreed();
        CHECK(SearchSteps == 227);
    }
}

TEST_CASE("FreeSolutionNodes After Failed Search") {
    AStarSearch<MapSearchNode> astarsearch;

    MapSearchNode nodeStart(0, 0);
    // (1, 1) has terrain value 9 (obstacle), which can never be reached
    MapSearchNode nodeEnd(1, 1);

    astarsearch.SetStartAndGoalStates(nodeStart, nodeEnd);

    unsigned int SearchState;
    do {
        SearchState = astarsearch.SearchStep();
    } while (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_SEARCHING);

    CHECK(SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_FAILED);

    // Calling FreeSolutionNodes after a failed search must be safe and not cause use-after-free
    astarsearch.FreeSolutionNodes();
    // Idempotent call
    astarsearch.FreeSolutionNodes();

    astarsearch.EnsureMemoryFreed();
}

TEST_CASE("Goal Node Costs Populated On Success") {
    AStarSearch<MapSearchNode> astarsearch;

    MapSearchNode nodeStart(0, 0);
    // (5, 0) is along row 0 and is reachable from (0, 0)
    MapSearchNode nodeEnd(5, 0);

    astarsearch.SetStartAndGoalStates(nodeStart, nodeEnd);

    unsigned int SearchState;
    do {
        SearchState = astarsearch.SearchStep();
    } while (SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_SEARCHING);

    CHECK(SearchState == AStarSearch<MapSearchNode>::SEARCH_STATE_SUCCEEDED);
    CHECK(astarsearch.GetSolutionCost() > 0.0f);

    MapSearchNode* goalNode = astarsearch.GetSolutionEnd();
    REQUIRE(goalNode != nullptr);
    CHECK(goalNode->x == 5);
    CHECK(goalNode->y == 0);

    astarsearch.FreeSolutionNodes();
    astarsearch.EnsureMemoryFreed();
}

struct DestructorTracker {
    static int alive_count;
    DestructorTracker() { alive_count++; }
    ~DestructorTracker() { alive_count--; }
};
int DestructorTracker::alive_count = 0;


class ReopenTestNode {
   public:
    int id;
    ReopenTestNode() : id(0) {}
    ReopenTestNode(int _id) : id(_id) {}

    float GoalDistanceEstimate(ReopenTestNode& goal) {
        if (id == 1) return 1.0f;   // B: heuristic 1 -> f = 10 + 1 = 11
        if (id == 2) return 12.0f;  // C: heuristic 12 -> f = 1 + 12 = 13
        if (id == 3) return 0.0f;   // Goal G
        return 10.0f;
    }
    bool IsGoal(ReopenTestNode& goal) { return id == goal.id; }
    bool GetSuccessors(AStarSearch<ReopenTestNode, false>* astar, ReopenTestNode* parent) {
        if (id == 0) {  // A -> B (cost 10), A -> C (cost 1)
            ReopenTestNode b(1), c(2);
            astar->AddSuccessor(b);
            astar->AddSuccessor(c);
        } else if (id == 1) {  // B -> G (cost 100)
            ReopenTestNode g(3);
            astar->AddSuccessor(g);
        } else if (id == 2) {  // C -> B (cost 1) which reopens B with cheaper cost (g=2 < 10)
            ReopenTestNode b(1);
            astar->AddSuccessor(b);
        }
        return true;
    }
    float GetCost(ReopenTestNode& successor) {
        if (id == 0 && successor.id == 1) return 10.0f;
        if (id == 0 && successor.id == 2) return 1.0f;
        if (id == 2 && successor.id == 1) return 1.0f;
        if (id == 1 && successor.id == 3) return 100.0f;
        return 1.0f;
    }
    bool IsSameState(ReopenTestNode& rhs) { return id == rhs.id; }
    size_t Hash() { return std::hash<int>{}(id); }
};

TEST_CASE("Reopen Node From Closed List When Cheaper Path Found") {
    AStarSearch<ReopenTestNode, false> astar;
    ReopenTestNode start(0);
    ReopenTestNode goal(3);

    astar.SetStartAndGoalStates(start, goal);

    unsigned int state;
    do {
        state = astar.SearchStep();
    } while (state == AStarSearch<ReopenTestNode>::SEARCH_STATE_SEARCHING);

    CHECK(state == AStarSearch<ReopenTestNode>::SEARCH_STATE_SUCCEEDED);
    // Path should be A(0) -> C(2) -> B(1) -> G(3) with total cost 1 + 1 + 100 = 102
    // rather than A(0) -> B(1) -> G(3) with cost 10 + 100 = 110
    CHECK(astar.GetSolutionCost() == doctest::Approx(102.0f));

    astar.FreeSolutionNodes();
    astar.EnsureMemoryFreed();
}


