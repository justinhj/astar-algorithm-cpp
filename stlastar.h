/*
A* Algorithm Implementation using STL is
Copyright (C)2001-2005 Justin Heyes-Jones

Permission is given by the author to freely redistribute and
include this code in any program as long as this credit is
given where due.

  COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
  INCLUDING, WITHOUT LIMITATION, WARRANTIES THAT THE COVERED CODE
  IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
  OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND
  PERFORMANCE OF THE COVERED CODE IS WITH YOU. SHOULD ANY COVERED
  CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT THE INITIAL
  DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY
  NECESSARY SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF
  WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS LICENSE. NO USE
  OF ANY COVERED CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
  THIS DISCLAIMER.

  Use at your own risk!

*/

#ifndef STLASTAR_H
#define STLASTAR_H
// used for text debugging
#include <assert.h>
#include <stdio.h>

// stl includes
#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <unordered_set>
#include <vector>

// fast fixed size memory allocator, used for fast node memory management
#include "fsa.h"

// Fixed size memory allocator can be disabled to compare performance
// Uses std new and delete instead if you turn it off
#define USE_FSA_MEMORY 1

// disable warning that debugging information has lines that are truncated
// occurs in stl headers
#if defined(WIN32) && defined(_WINDOWS)
#pragma warning(disable : 4786)
#endif

// The AStar search class. UserState is the users state space type
template <class UserState, bool ConsistentHeuristic = true>
class AStarSearch {
   public:  // data
    enum {
        SEARCH_STATE_NOT_INITIALISED,
        SEARCH_STATE_SEARCHING,
        SEARCH_STATE_SUCCEEDED,
        SEARCH_STATE_FAILED,
        SEARCH_STATE_OUT_OF_MEMORY,
        SEARCH_STATE_INVALID
    };

    // A node represents a possible state in the search
    // The user provided state type is included inside this type

   public:
    class Node {
       public:
        Node* parent;  // used during the search to record the parent of successor nodes
        Node* child;   // used after the search for the application to view the search in reverse

        float g;  // cost of this node + its predecessors
        float h;  // heuristic estimate of distance to goal
        float f;  // sum of cumulative cost of predecessors and self and heuristic

        size_t heap_index;  // index in m_OpenList, or SIZE_MAX when not on the heap

        Node()
            : parent(nullptr),
              child(nullptr),
              g(0.0f),
              h(0.0f),
              f(0.0f),
              heap_index(SIZE_MAX) {}

        bool operator==(const Node& otherNode) const {
            return this->m_UserState.IsSameState(otherNode.m_UserState);
        }

        UserState m_UserState;
    };

    // For sorting the heap the STL needs compare function that lets us compare
    // the f value of two nodes

    class HeapCompare_f {
       public:
        bool operator()(const Node* x, const Node* y) const {
            return x->f > y->f;
        }
    };

   public:  // methods
    // constructor just initialises private data
    AStarSearch()
        : m_State(SEARCH_STATE_NOT_INITIALISED),
          m_CurrentSolutionNode(nullptr),
#if USE_FSA_MEMORY
          m_FixedSizeAllocator(1000),
#endif
          m_AllocateNodeCount(0),
          m_CancelRequest(false),
          m_Start(nullptr),
          m_Goal(nullptr),
          m_CurrentExpandingNode(nullptr) {
    }

    AStarSearch(int MaxNodes)
        : m_State(SEARCH_STATE_NOT_INITIALISED),
          m_CurrentSolutionNode(nullptr),
#if USE_FSA_MEMORY
          m_FixedSizeAllocator(MaxNodes),
#endif
          m_AllocateNodeCount(0),
          m_CancelRequest(false),
          m_Start(nullptr),
          m_Goal(nullptr),
          m_CurrentExpandingNode(nullptr) {
    }

    AStarSearch(const AStarSearch&) = delete;
    AStarSearch& operator=(const AStarSearch&) = delete;

    ~AStarSearch() {
        if (m_State == SEARCH_STATE_SUCCEEDED) {
            FreeSolutionNodes();
        } else if (m_Start != nullptr) {
            FreeAllNodes();
        }
    }

    // call at any time to cancel the search and free up all the memory
    void CancelSearch() {
        m_CancelRequest = true;
    }

    // Set Start and goal states
    void SetStartAndGoalStates(UserState& Start, UserState& Goal) {
        if (m_Start != nullptr) {
            if (m_State == SEARCH_STATE_SUCCEEDED) {
                FreeSolutionNodes();
            } else {
                FreeAllNodes();
            }
        }

        m_CancelRequest = false;

        m_Start = AllocateNode();
        m_Goal = AllocateNode();

        assert((m_Start != nullptr && m_Goal != nullptr));

        m_Start->m_UserState = Start;
        m_Goal->m_UserState = Goal;

        m_State = SEARCH_STATE_SEARCHING;

        // Initialise the AStar specific parts of the Start Node
        // The user only needs fill out the state information

        m_Start->g = 0;
        m_Start->h = m_Start->m_UserState.GoalDistanceEstimate(m_Goal->m_UserState);
        m_Start->f = m_Start->g + m_Start->h;
        m_Start->parent = nullptr;

        // Push the start node on the Open list

        m_Start->heap_index = m_OpenList.size();
        m_OpenList.reserve(128);
        m_OpenList.push_back(m_Start);
        siftUp(m_Start->heap_index);

        m_NodeMap.insert(m_Start);
        AssertHeapInvariants();

        // Initialise counter for search steps
        m_Steps = 0;
    }

    // Advances search one step
    unsigned int SearchStep() {
        assert((m_State > SEARCH_STATE_NOT_INITIALISED) && (m_State < SEARCH_STATE_INVALID));

        if ((m_State == SEARCH_STATE_SUCCEEDED) || (m_State == SEARCH_STATE_FAILED)) {
            return m_State;
        }

        if (m_OpenList.empty() || m_CancelRequest) {
            FreeAllNodes();
            m_State = SEARCH_STATE_FAILED;
            return m_State;
        }

        m_Steps++;

        Node* n = m_OpenList.front();
        swapNodes(0, m_OpenList.size() - 1);
        m_OpenList.pop_back();
        n->heap_index = SIZE_MAX;
        if (!m_OpenList.empty()) {
            siftDown(0);
        }
        AssertHeapInvariants();

        if (n->m_UserState.IsGoal(m_Goal->m_UserState)) {
            m_Goal->parent = n->parent;
            m_Goal->g = n->g;
            m_Goal->h = n->h;
            m_Goal->f = n->f;

            if (false == n->m_UserState.IsSameState(m_Start->m_UserState)) {
                // m_NodeMap contains n, but we don't free it here, FreeUnusedNodes will handle it
                Node* nodeChild = m_Goal;
                Node* nodeParent = m_Goal->parent;
                do {
                    nodeParent->child = nodeChild;
                    nodeChild = nodeParent;
                    nodeParent = nodeParent->parent;
                } while (nodeChild != m_Start);
            }

            FreeUnusedNodes();
            m_State = SEARCH_STATE_SUCCEEDED;
            return m_State;
        } else {
            m_CurrentExpandingNode = n;
            bool ret = n->m_UserState.GetSuccessors(this, n->parent ? &n->parent->m_UserState : nullptr);
            m_CurrentExpandingNode = nullptr;

            if (!ret) {
                FreeAllNodes();
                m_State = SEARCH_STATE_OUT_OF_MEMORY;
                return m_State;
            }
        }

        return m_State;
    }

    // User calls this to add a successor to a list of successors
    // when expanding the search frontier
    bool AddSuccessor(UserState& State) {
        Node* n = m_CurrentExpandingNode;
        float newg = n->g + n->m_UserState.GetCost(State);

        Node dummy;
        dummy.m_UserState = State;
        auto map_result = m_NodeMap.find(&dummy);

        if (map_result != m_NodeMap.end()) {
            Node* existing = *map_result;
            if (existing->g <= newg) {
                return true;
            }

            if (existing->heap_index == SIZE_MAX) {
                if (ConsistentHeuristic) {
                    return true;
                }

                existing->parent = n;
                existing->g = newg;
                existing->h = existing->m_UserState.GoalDistanceEstimate(m_Goal->m_UserState);
                existing->f = existing->g + existing->h;

                existing->heap_index = m_OpenList.size();
                m_OpenList.push_back(existing);
                siftUp(existing->heap_index);
            } else {
                existing->parent = n;
                existing->g = newg;
                existing->f = existing->g + existing->h;
                siftUp(existing->heap_index);
            }
        } else {
            Node* successorNode = AllocateNode();
            if (!successorNode) {
                return false;
            }
            successorNode->m_UserState = State;
            successorNode->parent = n;
            successorNode->g = newg;
            successorNode->h = successorNode->m_UserState.GoalDistanceEstimate(m_Goal->m_UserState);
            successorNode->f = successorNode->g + successorNode->h;
            
            successorNode->heap_index = m_OpenList.size();
            m_OpenList.push_back(successorNode);
            siftUp(successorNode->heap_index);
            m_NodeMap.insert(successorNode);
        }
        return true;
    }

    // Free the solution nodes
    // This is done to clean up all used Node memory when you are done with the
    // search
    void FreeSolutionNodes() {
        if (m_State != SEARCH_STATE_SUCCEEDED || m_Start == nullptr) {
            return;
        }

        Node* n = m_Start;

        if (m_Start->child) {
            do {
                Node* del = n;
                n = n->child;
                FreeNode(del);

                del = nullptr;

            } while (n != m_Goal);

            FreeNode(n);  // Delete the goal

        } else {
            // if the start node is the solution we need to just delete the start and goal
            // nodes
            FreeNode(m_Start);
            FreeNode(m_Goal);
        }

        m_Start = nullptr;
        m_Goal = nullptr;
    }

    // Functions for traversing the solution

    // Get start node
    UserState* GetSolutionStart() {
        m_CurrentSolutionNode = m_Start;
        if (m_Start) {
            return &m_Start->m_UserState;
        } else {
            return nullptr;
        }
    }

    // Get next node
    UserState* GetSolutionNext() {
        if (m_CurrentSolutionNode) {
            if (m_CurrentSolutionNode->child) {
                Node* child = m_CurrentSolutionNode->child;

                m_CurrentSolutionNode = m_CurrentSolutionNode->child;

                return &child->m_UserState;
            }
        }

        return nullptr;
    }

    // Get end node
    UserState* GetSolutionEnd() {
        m_CurrentSolutionNode = m_Goal;
        if (m_Goal) {
            return &m_Goal->m_UserState;
        } else {
            return nullptr;
        }
    }

    // Step solution iterator backwards
    UserState* GetSolutionPrev() {
        if (m_CurrentSolutionNode) {
            if (m_CurrentSolutionNode->parent) {
                Node* parent = m_CurrentSolutionNode->parent;

                m_CurrentSolutionNode = m_CurrentSolutionNode->parent;

                return &parent->m_UserState;
            }
        }

        return nullptr;
    }

    // Get final cost of solution
    // Returns FLT_MAX if goal is not defined or there is no solution
    float GetSolutionCost() {
        if (m_Goal && m_State == SEARCH_STATE_SUCCEEDED) {
            return m_Goal->g;
        } else {
            return FLT_MAX;
        }
    }

    // For educational use and debugging it is useful to be able to view
    // the open and closed list at each step, here are two functions to allow that.

    UserState* GetOpenListStart() {
        float f, g, h;
        return GetOpenListStart(f, g, h);
    }

    UserState* GetOpenListStart(float& f, float& g, float& h) {
        iterDbgOpen = m_OpenList.begin();
        if (iterDbgOpen != m_OpenList.end()) {
            f = (*iterDbgOpen)->f;
            g = (*iterDbgOpen)->g;
            h = (*iterDbgOpen)->h;
            return &(*iterDbgOpen)->m_UserState;
        }

        return nullptr;
    }

    UserState* GetOpenListNext() {
        float f, g, h;
        return GetOpenListNext(f, g, h);
    }

    UserState* GetOpenListNext(float& f, float& g, float& h) {
        iterDbgOpen++;
        if (iterDbgOpen != m_OpenList.end()) {
            f = (*iterDbgOpen)->f;
            g = (*iterDbgOpen)->g;
            h = (*iterDbgOpen)->h;
            return &(*iterDbgOpen)->m_UserState;
        }

        return nullptr;
    }

    UserState* GetClosedListStart() {
        float f, g, h;
        return GetClosedListStart(f, g, h);
    }

    UserState* GetClosedListStart(float& f, float& g, float& h) {
        iterDbgNodeMap = m_NodeMap.begin();
        while (iterDbgNodeMap != m_NodeMap.end() && (*iterDbgNodeMap)->heap_index != SIZE_MAX) {
            ++iterDbgNodeMap;
        }
        if (iterDbgNodeMap != m_NodeMap.end()) {
            f = (*iterDbgNodeMap)->f;
            g = (*iterDbgNodeMap)->g;
            h = (*iterDbgNodeMap)->h;

            return &(*iterDbgNodeMap)->m_UserState;
        }

        return nullptr;
    }

    UserState* GetClosedListNext() {
        float f, g, h;
        return GetClosedListNext(f, g, h);
    }

    UserState* GetClosedListNext(float& f, float& g, float& h) {
        ++iterDbgNodeMap;
        while (iterDbgNodeMap != m_NodeMap.end() && (*iterDbgNodeMap)->heap_index != SIZE_MAX) {
            ++iterDbgNodeMap;
        }
        if (iterDbgNodeMap != m_NodeMap.end()) {
            f = (*iterDbgNodeMap)->f;
            g = (*iterDbgNodeMap)->g;
            h = (*iterDbgNodeMap)->h;

            return &(*iterDbgNodeMap)->m_UserState;
        }

        return nullptr;
    }

    // Get the number of steps

    int GetStepCount() {
        return m_Steps;
    }

    void EnsureMemoryFreed() {
        assert(m_AllocateNodeCount == 0);
    }

   private:  // methods
    void swapNodes(size_t i, size_t j) {
        if (i == j) return;
        std::swap(m_OpenList[i], m_OpenList[j]);
        m_OpenList[i]->heap_index = i;
        m_OpenList[j]->heap_index = j;
    }

    void siftUp(size_t i) {
        HeapCompare_f compare;
        while (i > 0) {
            size_t parent = (i - 1) / 2;
            if (compare(m_OpenList[parent], m_OpenList[i])) {
                swapNodes(parent, i);
                i = parent;
            } else {
                break;
            }
        }
    }

    void siftDown(size_t i) {
        HeapCompare_f compare;
        size_t size = m_OpenList.size();
        while (true) {
            size_t smallest = i;
            size_t left = 2 * i + 1;
            size_t right = 2 * i + 2;

            if (left < size && compare(m_OpenList[smallest], m_OpenList[left])) {
                smallest = left;
            }
            if (right < size && compare(m_OpenList[smallest], m_OpenList[right])) {
                smallest = right;
            }

            if (smallest != i) {
                swapNodes(i, smallest);
                i = smallest;
            } else {
                break;
            }
        }
    }

    void AssertHeapInvariants() const {
#if !defined(NDEBUG)
        HeapCompare_f compare;
        for (size_t i = 0; i < m_OpenList.size(); ++i) {
            assert(m_OpenList[i] != nullptr);
            assert(m_OpenList[i]->heap_index == i);
            size_t left = 2 * i + 1;
            size_t right = 2 * i + 2;
            if (left < m_OpenList.size()) {
                assert(!compare(m_OpenList[i], m_OpenList[left]));
            }
            if (right < m_OpenList.size()) {
                assert(!compare(m_OpenList[i], m_OpenList[right]));
            }
        }
#endif
    }

    // This is called when a search fails or is cancelled to free all used
    // memory
    void FreeAllNodes() {
        for (auto n : m_NodeMap) {
            FreeNode(n);
        }
        m_NodeMap.clear();
        m_OpenList.clear();

        FreeNode(m_Goal);
        m_Start = nullptr;
        m_Goal = nullptr;
    }

    // This call is made by the search class when the search ends. A lot of nodes may be
    // created that are still present when the search ends. They will be deleted by this
    // routine once the search ends
    void FreeUnusedNodes() {
        for (auto n : m_NodeMap) {
            if (n != m_Start && !n->child) {
                FreeNode(n);
            }
        }
        m_NodeMap.clear();
        m_OpenList.clear();
    }

    // Node memory management
    Node* AllocateNode() {
#if !USE_FSA_MEMORY
        m_AllocateNodeCount++;
        Node* p = new Node;
        return p;
#else
        Node* address = m_FixedSizeAllocator.alloc();

        if (!address) {
            return nullptr;
        }
        m_AllocateNodeCount++;
        Node* p = new (address) Node;
        return p;
#endif
    }

    void FreeNode(Node* node) {
        m_AllocateNodeCount--;

#if !USE_FSA_MEMORY
        delete node;
#else
        node->~Node();
        m_FixedSizeAllocator.free(node);
#endif
    }

   private:  // data
    // Heap (simple vector but used as a heap, cf. Steve Rabin's game gems article)
    std::vector<Node*> m_OpenList;

    // Closed is an unordered_set
    struct NodeHash {
        size_t operator()(Node* const& n) const {
            return n->m_UserState.Hash();
        }
    };
    struct NodeEqual {
        bool operator()(Node* a, Node* b) const {
            return a->m_UserState.IsSameState(b->m_UserState);
        }
    };
    
    std::unordered_set<Node*, NodeHash, NodeEqual> m_NodeMap;

    // Successors is a vector filled out by the user each type successors to a node
    // are generated
    Node* m_CurrentExpandingNode;

    // State
    unsigned int m_State;

    // Counts steps
    int m_Steps;

    // Start and goal state pointers
    Node* m_Start;
    Node* m_Goal;

    Node* m_CurrentSolutionNode;

#if USE_FSA_MEMORY
    // Memory
    FixedSizeAllocator<Node> m_FixedSizeAllocator;
#endif

    // Debug : need to keep these two iterators around
    //  for the user Dbg functions
    typename std::vector<Node*>::iterator iterDbgOpen;
    typename std::unordered_set<Node*, NodeHash, NodeEqual>::iterator iterDbgNodeMap;

    // debugging : count memory allocation and free's
    int m_AllocateNodeCount;

    bool m_CancelRequest;
};

#endif
