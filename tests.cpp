// A unit test suite

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

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

struct TrackedUserState {
    int id;
    std::shared_ptr<DestructorTracker> tracker;

    TrackedUserState() : id(0), tracker(std::make_shared<DestructorTracker>()) {}
    TrackedUserState(int _id) : id(_id), tracker(std::make_shared<DestructorTracker>()) {}

    float GoalDistanceEstimate(TrackedUserState& goal) { return (float)std::abs(id - goal.id); }
    bool IsGoal(TrackedUserState& goal) { return id == goal.id; }
    bool GetSuccessors(AStarSearch<TrackedUserState>* astar, TrackedUserState* parent) {
        if (id < 3) {
            TrackedUserState next(id + 1);
            astar->AddSuccessor(next);
        }
        return true;
    }
    float GetCost(TrackedUserState& successor) { return 1.0f; }
    bool IsSameState(TrackedUserState& rhs) { return id == rhs.id; }
    size_t Hash() { return std::hash<int>{}(id); }
};


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

TEST_CASE("Start Node Equals Goal Node") {
    AStarSearch<MapSearchNode> astar;
    MapSearchNode start(1, 1);
    MapSearchNode goal(1, 1);

    astar.SetStartAndGoalStates(start, goal);

    unsigned int state = astar.SearchStep();
    CHECK(state == AStarSearch<MapSearchNode>::SEARCH_STATE_SUCCEEDED);

    MapSearchNode* solStart = astar.GetSolutionStart();
    REQUIRE(solStart != nullptr);
    CHECK(solStart->x == 1);
    CHECK(solStart->y == 1);
    CHECK(astar.GetSolutionNext() == nullptr);
    CHECK(astar.GetSolutionCost() == 0.0f);

    astar.FreeSolutionNodes();
    astar.EnsureMemoryFreed();
}

TEST_CASE("AStarSearch Destructor Cleans Up Live Nodes On Scope Exit Without Manual Free") {
    DestructorTracker::alive_count = 0;
    {
        AStarSearch<TrackedUserState> astar;
        TrackedUserState start(0);
        TrackedUserState goal(2);
        astar.SetStartAndGoalStates(start, goal);

        unsigned int state;
        do {
            state = astar.SearchStep();
        } while (state == AStarSearch<TrackedUserState>::SEARCH_STATE_SEARCHING);

        CHECK(state == AStarSearch<TrackedUserState>::SEARCH_STATE_SUCCEEDED);
        // FreeSolutionNodes() is intentionally NOT called here to test destructor RAII cleanup
    }
    CHECK(DestructorTracker::alive_count == 0);
}

TEST_CASE("AStarSearch Destructor Cleans Up Unfinished Search On Scope Exit") {
    DestructorTracker::alive_count = 0;
    {
        AStarSearch<TrackedUserState> astar;
        TrackedUserState start(0);
        TrackedUserState goal(2);
        astar.SetStartAndGoalStates(start, goal);
        // Execute only one step; search remains in progress
        astar.SearchStep();
    }
    CHECK(DestructorTracker::alive_count == 0);
}

TEST_CASE("SetStartAndGoalStates Cleans Up Previous Search On Reuse") {
    AStarSearch<MapSearchNode> astar;
    MapSearchNode start1(0, 0);
    MapSearchNode goal1(1, 0);
    astar.SetStartAndGoalStates(start1, goal1);
    unsigned int state1;
    do {
        state1 = astar.SearchStep();
    } while (state1 == AStarSearch<MapSearchNode>::SEARCH_STATE_SEARCHING);
    CHECK(state1 == AStarSearch<MapSearchNode>::SEARCH_STATE_SUCCEEDED);

    // Reuse without calling FreeSolutionNodes()
    MapSearchNode start2(1, 1);
    MapSearchNode goal2(1, 1);
    astar.SetStartAndGoalStates(start2, goal2);
    unsigned int state2 = astar.SearchStep();
    CHECK(state2 == AStarSearch<MapSearchNode>::SEARCH_STATE_SUCCEEDED);

    astar.FreeSolutionNodes();
    astar.EnsureMemoryFreed();
}

TEST_CASE("AStarSearch and FixedSizeAllocator Are Non-Copyable") {
    CHECK(!std::is_copy_constructible<AStarSearch<MapSearchNode>>::value);
    CHECK(!std::is_copy_assignable<AStarSearch<MapSearchNode>>::value);
    CHECK(!std::is_copy_constructible<FixedSizeAllocator<int>>::value);
    CHECK(!std::is_copy_assignable<FixedSizeAllocator<int>>::value);
}

namespace {

struct WeightedEdge {
    int to;
    float cost;
};

struct WeightedNode {
    float x;
    float y;
    std::vector<WeightedEdge> edges;
};

struct WeightedGraph {
    std::vector<WeightedNode> nodes;

    float Euclidean(int u, int v) const {
        float dx = nodes[u].x - nodes[v].x;
        float dy = nodes[u].y - nodes[v].y;
        return std::sqrt(dx * dx + dy * dy);
    }

    float EdgeCost(int u, int v) const {
        for (size_t i = 0; i < nodes[u].edges.size(); ++i) {
            if (nodes[u].edges[i].to == v) {
                return nodes[u].edges[i].cost;
            }
        }
        return std::numeric_limits<float>::max();
    }

    bool HasEdge(int u, int v) const {
        for (size_t i = 0; i < nodes[u].edges.size(); ++i) {
            if (nodes[u].edges[i].to == v) return true;
        }
        return false;
    }

    void AddUndirectedEdge(int u, int v, float weight_factor) {
        if (u == v || HasEdge(u, v)) return;
        float cost = Euclidean(u, v) * weight_factor;
        nodes[u].edges.push_back(WeightedEdge{v, cost});
        nodes[v].edges.push_back(WeightedEdge{u, cost});
    }
};

WeightedGraph GenerateConnectedWeightedGraph(int num_nodes, uint32_t seed) {
    WeightedGraph g;
    g.nodes.resize(num_nodes);
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> pos_dist(0.0f, 1000.0f);
    std::uniform_real_distribution<float> weight_dist(1.0f, 4.2f);

    for (int i = 0; i < num_nodes; ++i) {
        g.nodes[i].x = pos_dist(rng);
        g.nodes[i].y = pos_dist(rng);
    }

    // Connect each node to its nearest neighbors, then bridge components so it is fully connected
    for (int u = 0; u < num_nodes; ++u) {
        std::vector<std::pair<float, int> > nearest;
        nearest.reserve(num_nodes - 1);
        for (int v = 0; v < num_nodes; ++v) {
            if (u == v) continue;
            nearest.push_back(std::make_pair(g.Euclidean(u, v), v));
        }
        std::sort(nearest.begin(), nearest.end());
        int degree = std::min(4, static_cast<int>(nearest.size()));
        for (int k = 0; k < degree; ++k) {
            g.AddUndirectedEdge(u, nearest[k].second, weight_dist(rng));
        }
    }

    while (true) {
        std::vector<bool> vis(num_nodes, false);
        std::queue<int> q;
        vis[0] = true;
        q.push(0);
        int seen = 0;
        while (!q.empty()) {
            int cur = q.front();
            q.pop();
            ++seen;
            for (size_t i = 0; i < g.nodes[cur].edges.size(); ++i) {
                int nb = g.nodes[cur].edges[i].to;
                if (!vis[nb]) {
                    vis[nb] = true;
                    q.push(nb);
                }
            }
        }
        if (seen == num_nodes) break;

        int best_u = -1;
        int best_v = -1;
        float best_d = std::numeric_limits<float>::max();
        for (int u = 0; u < num_nodes; ++u) {
            if (!vis[u]) continue;
            for (int v = 0; v < num_nodes; ++v) {
                if (vis[v]) continue;
                float d = g.Euclidean(u, v);
                if (d < best_d) {
                    best_d = d;
                    best_u = u;
                    best_v = v;
                }
            }
        }
        g.AddUndirectedEdge(best_u, best_v, weight_dist(rng));
    }

    return g;
}

float DijkstraShortestPathCost(const WeightedGraph& g, int start, int goal) {
    const int n = static_cast<int>(g.nodes.size());
    std::vector<float> dist(n, std::numeric_limits<float>::max());
    typedef std::pair<float, int> QueueItem;
    std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem> > pq;

    dist[start] = 0.0f;
    pq.push(std::make_pair(0.0f, start));

    while (!pq.empty()) {
        QueueItem top = pq.top();
        pq.pop();
        float d = top.first;
        int u = top.second;
        if (d > dist[u]) continue;
        if (u == goal) return d;

        for (size_t i = 0; i < g.nodes[u].edges.size(); ++i) {
            const WeightedEdge& e = g.nodes[u].edges[i];
            float nd = d + e.cost;
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                pq.push(std::make_pair(nd, e.to));
            }
        }
    }
    return std::numeric_limits<float>::max();
}

struct WeightedGraphSearchNode {
    int id;
    static const WeightedGraph* s_Graph;

    WeightedGraphSearchNode() : id(0) {}
    explicit WeightedGraphSearchNode(int node_id) : id(node_id) {}

    float GoalDistanceEstimate(WeightedGraphSearchNode& nodeGoal) {
        return s_Graph->Euclidean(id, nodeGoal.id);
    }

    bool IsGoal(WeightedGraphSearchNode& nodeGoal) {
        return id == nodeGoal.id;
    }

    bool GetSuccessors(AStarSearch<WeightedGraphSearchNode>* astarsearch,
                       WeightedGraphSearchNode* parent_node) {
        int parent_id = parent_node ? parent_node->id : -1;
        const std::vector<WeightedEdge>& edges = s_Graph->nodes[id].edges;
        for (size_t i = 0; i < edges.size(); ++i) {
            if (edges[i].to == parent_id) continue;
            WeightedGraphSearchNode succ(edges[i].to);
            if (!astarsearch->AddSuccessor(succ)) {
                return false;
            }
        }
        return true;
    }

    float GetCost(WeightedGraphSearchNode& successor) {
        return s_Graph->EdgeCost(id, successor.id);
    }

    bool IsSameState(WeightedGraphSearchNode& rhs) {
        return id == rhs.id;
    }

    size_t Hash() {
        return std::hash<int>()(id);
    }
};

const WeightedGraph* WeightedGraphSearchNode::s_Graph = nullptr;

}  // namespace

TEST_CASE("Weighted Graph Matches Dijkstra Oracle") {
    const int sizes[] = {16, 48, 120};
    const uint32_t seeds[] = {101u, 2029u, 99991u};

    for (size_t si = 0; si < sizeof(sizes) / sizeof(sizes[0]); ++si) {
        int num_nodes = sizes[si];
        for (size_t ki = 0; ki < sizeof(seeds) / sizeof(seeds[0]); ++ki) {
            uint32_t seed = seeds[ki] + static_cast<uint32_t>(num_nodes);
            WeightedGraph graph = GenerateConnectedWeightedGraph(num_nodes, seed);
            WeightedGraphSearchNode::s_Graph = &graph;

            int start_id = 0;
            int goal_id = num_nodes - 1;
            float expected_cost = DijkstraShortestPathCost(graph, start_id, goal_id);

            AStarSearch<WeightedGraphSearchNode> astar(2048);
            WeightedGraphSearchNode start(start_id);
            WeightedGraphSearchNode goal(goal_id);
            astar.SetStartAndGoalStates(start, goal);

            unsigned int state;
            do {
                state = astar.SearchStep();
            } while (state == AStarSearch<WeightedGraphSearchNode>::SEARCH_STATE_SEARCHING);

            REQUIRE(state == AStarSearch<WeightedGraphSearchNode>::SEARCH_STATE_SUCCEEDED);
            CHECK(astar.GetSolutionCost() == doctest::Approx(expected_cost).epsilon(0.001));

            // Walk solution path and verify edge validity and summed cost
            WeightedGraphSearchNode* cur = astar.GetSolutionStart();
            REQUIRE(cur != nullptr);
            CHECK(cur->id == start_id);

            float walked_cost = 0.0f;
            int prev_id = cur->id;
            while (WeightedGraphSearchNode* next = astar.GetSolutionNext()) {
                REQUIRE(graph.HasEdge(prev_id, next->id));
                walked_cost += graph.EdgeCost(prev_id, next->id);
                prev_id = next->id;
            }
            CHECK(prev_id == goal_id);
            CHECK(walked_cost == doctest::Approx(expected_cost).epsilon(0.001));

            astar.FreeSolutionNodes();
            astar.EnsureMemoryFreed();
        }
    }
}


