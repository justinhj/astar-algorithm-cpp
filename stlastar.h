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
template <class UserState>
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
          m_Goal(nullptr) {
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
          m_Goal(nullptr) {
    }

    // call at any time to cancel the search and free up all the memory
    void CancelSearch() {
        m_CancelRequest = true;
    }

    // Set Start and goal states
    void SetStartAndGoalStates(UserState& Start, UserState& Goal) {
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
        m_OpenList.push_back(m_Start);
        siftUp(m_Start->heap_index);

        m_OpenSet.insert(m_Start);
        AssertHeapInvariants();

        // Initialise counter for search steps
        m_Steps = 0;
    }

    // Advances search one step
    unsigned int SearchStep() {
        // Firstly break if the user has not initialised the search
        assert((m_State > SEARCH_STATE_NOT_INITIALISED) && (m_State < SEARCH_STATE_INVALID));

        // Next I want it to be safe to do a searchstep once the search has succeeded...
        if ((m_State == SEARCH_STATE_SUCCEEDED) || (m_State == SEARCH_STATE_FAILED)) {
            return m_State;
        }

        // Failure is defined as emptying the open list as there is nothing left to
        // search...
        // New: Allow user abort
        if (m_OpenList.empty() || m_CancelRequest) {
            FreeAllNodes();
            m_State = SEARCH_STATE_FAILED;
            return m_State;
        }

        // Incremement step count
        m_Steps++;

        // Pop the best node (the one with the lowest f)
        Node* n = m_OpenList.front();  // get pointer to the node
        swapNodes(0, m_OpenList.size() - 1);
        m_OpenList.pop_back();
        n->heap_index = SIZE_MAX;
        if (!m_OpenList.empty()) {
            siftDown(0);
        }
        m_OpenSet.erase(n);
        AssertHeapInvariants();

        // Check for the goal, once we pop that we're done
        if (n->m_UserState.IsGoal(m_Goal->m_UserState)) {
            // The user is going to use the Goal Node he passed in
            // so copy the parent pointer and costs of n
            m_Goal->parent = n->parent;
            m_Goal->g = n->g;
            m_Goal->h = n->h;
            m_Goal->f = n->f;

            // A special case is that the goal was passed in as the start state
            // so handle that here
            if (false == n->m_UserState.IsSameState(m_Start->m_UserState)) {
                FreeNode(n);

                // set the child pointers in each node (except Goal which has no child)
                Node* nodeChild = m_Goal;
                Node* nodeParent = m_Goal->parent;

                do {
                    nodeParent->child = nodeChild;

                    nodeChild = nodeParent;
                    nodeParent = nodeParent->parent;

                } while (nodeChild != m_Start);  // Start is always the first node by definition
            }

            // delete nodes that aren't needed for the solution
            FreeUnusedNodes();

            m_State = SEARCH_STATE_SUCCEEDED;

            return m_State;
        } else  // not goal
        {
            // We now need to generate the successors of this node
            // The user helps us to do this, and we keep the new nodes in
            // m_Successors ...

            m_Successors.clear();  // empty vector of successor nodes to n

            // User provides this functions and uses AddSuccessor to add each successor of
            // node 'n' to m_Successors
            bool ret =
                n->m_UserState.GetSuccessors(this, n->parent ? &n->parent->m_UserState : nullptr);

            if (!ret) {
                typename std::vector<Node*>::iterator successor;

                // free the nodes that may previously have been added
                for (successor = m_Successors.begin(); successor != m_Successors.end();
                     successor++) {
                    FreeNode((*successor));
                }

                m_Successors.clear();  // empty vector of successor nodes to n

                // free up everything else we allocated
                FreeNode((n));
                FreeAllNodes();

                m_State = SEARCH_STATE_OUT_OF_MEMORY;
                return m_State;
            }

            // Now handle each successor to the current node ...
            for (typename std::vector<Node*>::iterator successor = m_Successors.begin();
                 successor != m_Successors.end(); successor++) {
                // 	The g value for this successor ...
                float newg = n->g + n->m_UserState.GetCost((*successor)->m_UserState);

                // Now we need to find whether the node is on the open or closed lists
                // If it is but the node that is already on them is better (lower g)
                // then we can forget about this successor

                typename std::unordered_set<Node*, NodeHash, NodeEqual>::iterator openlist_result;
                openlist_result = m_OpenSet.find(*successor);

                if (openlist_result != m_OpenSet.end()) {
                    // we found this state on open

                    if ((*openlist_result)->g <= newg) {
                        FreeNode((*successor));

                        // the one on Open is cheaper than this one
                        continue;
                    }
                }
                typename std::unordered_set<Node*, NodeHash, NodeEqual>::iterator closedlist_result;

                closedlist_result = m_ClosedList.find(*successor);

                if (closedlist_result != m_ClosedList.end()) {
                    // we found this state on closed

                    if ((*closedlist_result)->g <= newg) {
                        // the one on Closed is cheaper than this one
                        FreeNode((*successor));

                        continue;
                    }
                }

                // This node is the best node so far with this particular state
                // so lets keep it and set up its AStar specific data ...

                (*successor)->parent = n;
                (*successor)->g = newg;
                (*successor)->h =
                    (*successor)->m_UserState.GoalDistanceEstimate(m_Goal->m_UserState);
                (*successor)->f = (*successor)->g + (*successor)->h;

                // Successor in closed list
                // 1 - Update old version of this node in closed list
                // 2 - Move it from closed to open list
                // 3 - Sort heap again in open list

                if (closedlist_result != m_ClosedList.end()) {
                    // Update closed node with successor node AStar data
                    //*(*closedlist_result) = *(*successor);
                    (*closedlist_result)->parent = (*successor)->parent;
                    (*closedlist_result)->g = (*successor)->g;
                    (*closedlist_result)->h = (*successor)->h;
                    (*closedlist_result)->f = (*successor)->f;

                    // Free successor node
                    FreeNode((*successor));

                    // Push closed node into open list
                    (*closedlist_result)->heap_index = m_OpenList.size();
                    m_OpenList.push_back((*closedlist_result));

                    // Remove closed node from closed list
                    m_ClosedList.erase(closedlist_result);

                    siftUp((*closedlist_result)->heap_index);

                    // Add to open set
                    m_OpenSet.insert(*closedlist_result);
                    AssertHeapInvariants();

                    // Fix thanks to ...
                    // Greg Douglas <gregdouglasmail@gmail.com>
                    // who noticed that this code path was incorrect
                    // Here we have found a new state which is already CLOSED

                }

                // Successor in open list
                // 1 - Update old version of this node in open list
                // 2 - sort heap again in open list

                else if (openlist_result != m_OpenSet.end()) {
                    // Update open node with successor node AStar data
                    //*(*openlist_result) = *(*successor);
                    (*openlist_result)->parent = (*successor)->parent;
                    (*openlist_result)->g = (*successor)->g;
                    (*openlist_result)->h = (*successor)->h;
                    (*openlist_result)->f = (*successor)->f;

                    // Free successor node
                    FreeNode((*successor));

                    siftUp((*openlist_result)->heap_index);
                    AssertHeapInvariants();
                }

                // New successor
                // 1 - Move it from successors to open list
                // 2 - sort heap again in open list

                else {
                    // Push successor node into open list
                    (*successor)->heap_index = m_OpenList.size();
                    m_OpenList.push_back((*successor));

                    siftUp((*successor)->heap_index);

                    // Add to open set
                    m_OpenSet.insert(*successor);
                    AssertHeapInvariants();
                }
            }

            // push n onto Closed, as we have expanded it now

            m_ClosedList.insert(n);

        }  // end else (not goal so expand)

        return m_State;  // Succeeded bool is false at this point.
    }

    // User calls this to add a successor to a list of successors
    // when expanding the search frontier
    bool AddSuccessor(UserState& State) {
        Node* node = AllocateNode();

        if (node) {
            node->m_UserState = State;

            m_Successors.push_back(node);

            return true;
        }

        return false;
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
        iterDbgClosed = m_ClosedList.begin();
        if (iterDbgClosed != m_ClosedList.end()) {
            f = (*iterDbgClosed)->f;
            g = (*iterDbgClosed)->g;
            h = (*iterDbgClosed)->h;

            return &(*iterDbgClosed)->m_UserState;
        }

        return nullptr;
    }

    UserState* GetClosedListNext() {
        float f, g, h;
        return GetClosedListNext(f, g, h);
    }

    UserState* GetClosedListNext(float& f, float& g, float& h) {
        iterDbgClosed++;
        if (iterDbgClosed != m_ClosedList.end()) {
            f = (*iterDbgClosed)->f;
            g = (*iterDbgClosed)->g;
            h = (*iterDbgClosed)->h;

            return &(*iterDbgClosed)->m_UserState;
        }

        return nullptr;
    }

    // Get the number of steps

    int GetStepCount() {
        return m_Steps;
    }

    void EnsureMemoryFreed() {
#if USE_FSA_MEMORY
        assert(m_AllocateNodeCount == 0);
#endif
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
        // iterate open list and delete all nodes
        typename std::vector<Node*>::iterator iterOpen = m_OpenList.begin();

        while (iterOpen != m_OpenList.end()) {
            Node* n = (*iterOpen);
            n->heap_index = SIZE_MAX;
            FreeNode(n);

            iterOpen++;
        }

        m_OpenList.clear();
        m_OpenSet.clear();

        // iterate closed list and delete unused nodes
        typename std::unordered_set<Node*, NodeHash, NodeEqual>::iterator iterClosed;

        for (iterClosed = m_ClosedList.begin(); iterClosed != m_ClosedList.end(); iterClosed++) {
            Node* n = (*iterClosed);
            FreeNode(n);
        }

        m_ClosedList.clear();

        // delete the goal

        FreeNode(m_Goal);

        m_Start = nullptr;
        m_Goal = nullptr;
    }

    // This call is made by the search class when the search ends. A lot of nodes may be
    // created that are still present when the search ends. They will be deleted by this
    // routine once the search ends
    void FreeUnusedNodes() {
        // iterate open list and delete unused nodes
        typename std::vector<Node*>::iterator iterOpen = m_OpenList.begin();

        while (iterOpen != m_OpenList.end()) {
            Node* n = (*iterOpen);
            n->heap_index = SIZE_MAX;

            if (!n->child) {
                FreeNode(n);

                n = nullptr;
            }

            iterOpen++;
        }

        m_OpenList.clear();
        m_OpenSet.clear();

        // iterate closed list and delete unused nodes
        typename std::unordered_set<Node*, NodeHash, NodeEqual>::iterator iterClosed;

        for (iterClosed = m_ClosedList.begin(); iterClosed != m_ClosedList.end(); iterClosed++) {
            Node* n = (*iterClosed);

            if (!n->child) {
                FreeNode(n);
                n = nullptr;
            }
        }

        m_ClosedList.clear();
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
    std::unordered_set<Node*, NodeHash, NodeEqual> m_ClosedList;
    std::unordered_set<Node*, NodeHash, NodeEqual> m_OpenSet;

    // Successors is a vector filled out by the user each type successors to a node
    // are generated
    std::vector<Node*> m_Successors;

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
    typename std::unordered_set<Node*, NodeHash, NodeEqual>::iterator iterDbgClosed;

    // debugging : count memory allocation and free's
    int m_AllocateNodeCount;

    bool m_CancelRequest;
};

#endif
