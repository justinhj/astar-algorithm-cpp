using System.Collections;
using System.Collections.Generic;


// Converted to C# originally for use with Unity, hence some apparently odd
// conventions: foreach is not used as that generates garbage in Unity.
// Preallocated lists are used to preclude any runtime allocations.
// The current implementation is no longer dependent on Unity (but will work within it of course).

public class AStarPathfinder
{
	public enum SearchState
	{
		NotInitialized,
		Searching,
		Succeeded,
		Failed,
		OutOfMemory,
		Invalid
	}

	// A node represents a possible state in the search
	// The user provided state type is included inside this type
	class Node
	{
		public Node parent; // used during the search to record the parent of successor nodes
		public Node child; // used after the search for the application to view the search in reverse

		public float g; // cost of this node + it's predecessors
		public float h; // heuristic estimate of distance to goal
		public float f; // sum of cumulative cost of predecessors and self and heuristic

		public Node()
		{
			Reinitialize();
		}

		public void Reinitialize()
		{
			parent = null;
			child = null;
			g = 0.0f;
			h = 0.0f;
			f = 0.0f;
		}

		public MapSearchNode m_UserState;
	};


	// Heap (simple list but used as a heap, cf. Steve Rabin's game gems article)
	List<Node> m_OpenList;

	// Closed list is a list.
	List<Node> m_ClosedList; 

	// Successors is a list filled out by the user each type successors to a node are generated
	List<Node> m_Successors;

	// State
	SearchState m_State = SearchState.NotInitialized;

	// Counts steps
	int m_Steps = 0;

	// Start and goal state pointers
	Node m_Start = null;
	Node m_Goal = null;

	Node m_CurrentSolutionNode = null;

	// Memory
 	List<Node> m_FixedSizeAllocator;

	int m_AllocateNodeCount = 0;

	bool m_CancelRequest = false;

	int allocatedMapSearchNodes = 0;

	List<MapSearchNode> mapSearchNodePool = null;

	int openListHighWaterMark = 0;
	int closedListHighWaterMark = 0;
	int successorListHighWaterMark = 0;

	// Fixed sizes for collections
	readonly int kPreallocatedNodes = 4000;
	readonly int kPreallocatedMapSearchNodes = 1000;

	readonly int kPreallocatedOpenListSlots = 32;
	readonly int kPreallocatedClosedListSlots = 256;
	readonly int kPreallocatedSuccessorSlots = 8;


	// constructor just initialises private data
	public AStarPathfinder()
	{
		// Allocate all lists		
		m_OpenList = new List<Node>(kPreallocatedOpenListSlots);
		m_ClosedList = new List<Node>(kPreallocatedClosedListSlots);
		m_Successors = new List<Node>(kPreallocatedSuccessorSlots);

		m_FixedSizeAllocator = new List<Node>(kPreallocatedNodes);
		for (int i = 0; i < kPreallocatedNodes; ++i)
		{
			Node n = new Node();
			m_FixedSizeAllocator.Add(n);
		}

		mapSearchNodePool = new List<MapSearchNode>(kPreallocatedMapSearchNodes);
		for (int i = 0; i < kPreallocatedMapSearchNodes; ++i)
		{
			MapSearchNode msn = new MapSearchNode(this);
			mapSearchNodePool.Add(msn);
		}
	}

	// call at any time to cancel the search and free up all the memory
	public void CancelSearch()
	{
		m_CancelRequest = true;
	}

	// Build the open list as sorted to begin with by inserting new elements in the right place
	void SortedAddToOpenList(Node node)
	{
		bool inserted = false;

		for (int i = 0; i < m_OpenList.Count; ++i)
		{
			if (node.f < m_OpenList[i].f)
			{
				m_OpenList.Insert(i, node);
				break;
			}
		}

		if (!inserted)
		{
			m_OpenList.Add(node);
		}

		if (m_OpenList.Count > openListHighWaterMark)
		{
			openListHighWaterMark = m_OpenList.Count;
		}
	}

	Node AllocateNode()
	{
		if (m_AllocateNodeCount >= kPreallocatedNodes)
		{
			System.Console.WriteLine("FATAL - Pathfinder ran out of preallocated nodes!");
		}

		return m_FixedSizeAllocator[m_AllocateNodeCount++];
	}

	public MapSearchNode AllocateMapSearchNode(NodePosition nodePosition)
	{
		if (allocatedMapSearchNodes >= kPreallocatedMapSearchNodes)
		{
			System.Console.WriteLine("FATAL - HexGrid has run out of preallocated MapSearchNodes!");
		}

		mapSearchNodePool[allocatedMapSearchNodes].position = nodePosition;
		return mapSearchNodePool[allocatedMapSearchNodes++];
	}

	public void InitiatePathfind()
	{
		m_CancelRequest = false;
		m_AllocateNodeCount = 0;	// Reset our used node tracking
		allocatedMapSearchNodes = 0;
	}

	// Set Start and goal states
	public void SetStartAndGoalStates(MapSearchNode Start, MapSearchNode Goal)
	{
		m_Start = AllocateNode();
		m_Goal = AllocateNode();

		System.Diagnostics.Debug.Assert((m_Start != null && m_Goal != null));

		m_Start.m_UserState = Start;
		m_Goal.m_UserState = Goal;

		m_State = SearchState.Searching;

		// Initialise the AStar specific parts of the Start Node
		// The user only needs fill out the state information
		m_Start.g = 0; 
		m_Start.h = m_Start.m_UserState.GoalDistanceEstimate( m_Goal.m_UserState );
		m_Start.f = m_Start.g + m_Start.h;
		m_Start.parent = null;

		// Push the start node on the Open list
		m_OpenList.Add(m_Start);

		// Initialise counter for search steps
		m_Steps = 0;

#if PATHFIND_DEBUG
		System.Console.WriteLine("Starting pathfind. Start: " + m_Start.m_UserState.position + ", Goal: " + m_Goal.m_UserState.position);
#endif
	}

	// Advances search one step 
	public SearchState SearchStep()
	{
		// Firstly break if the user has not initialised the search
		System.Diagnostics.Debug.Assert((m_State > SearchState.NotInitialized) && (m_State < SearchState.Invalid));

		// Next I want it to be safe to do a searchstep once the search has succeeded...
		if (m_State == SearchState.Succeeded || m_State == SearchState.Failed)
		{
			return m_State; 
		}

		// Failure is defined as emptying the open list as there is nothing left to 
		// search...
		// New: Allow user abort
		if (m_OpenList.Count == 0 || m_CancelRequest)
		{
			FreeSolutionNodes();
			m_State = SearchState.Failed;
			return m_State;
		}

		// Incremement step count
		m_Steps++;

		// Pop the best node (the one with the lowest f) 
		Node n = m_OpenList[0]; // get pointer to the node
		m_OpenList.RemoveAt(0);

		//System.Console.WriteLine("Checking node at " + n.m_UserState.position + ", f: " + n.f);

		// Check for the goal, once we pop that we're done
		if( n.m_UserState.IsGoal( m_Goal.m_UserState ) )
		{
			// The user is going to use the Goal Node he passed in 
			// so copy the parent pointer of n 
			m_Goal.parent = n.parent;
			m_Goal.g = n.g;

			// A special case is that the goal was passed in as the start state
			// so handle that here
			if( false == n.m_UserState.IsSameState( m_Start.m_UserState ) )
			{
				// set the child pointers in each node (except Goal which has no child)
				Node nodeChild = m_Goal;
				Node nodeParent = m_Goal.parent;

				do 
				{
					nodeParent.child = nodeChild;
					nodeChild = nodeParent;
					nodeParent = nodeParent.parent;
				} 
				while( nodeChild != m_Start ); // Start is always the first node by definition
			}

			// delete nodes that aren't needed for the solution
			//FreeUnusedNodes();

#if PATHFIND_DEBUG
			System.Console.WriteLine("GOAL REACHED! Steps: " + m_Steps + ", allocated nodes: " + m_AllocateNodeCount + ", MapSearchNodes: " + allocatedMapSearchNodes);
			System.Console.WriteLine("High water marks - Open:" + openListHighWaterMark + ", Closed: " + closedListHighWaterMark + ", Successors: " + successorListHighWaterMark);
#endif

			m_State = SearchState.Succeeded;
			return m_State;
		}
		else // not goal
		{
			// We now need to generate the successors of this node
			// The user helps us to do this, and we keep the new nodes in m_Successors ...
			m_Successors.Clear(); // empty vector of successor nodes to n

			// User provides this functions and uses AddSuccessor to add each successor of
			// node 'n' to m_Successors
			bool ret = false;
			if (n.parent != null)
			{
				ret = n.m_UserState.GetSuccessors(this, n.parent.m_UserState);
			}
			else
			{
				ret = n.m_UserState.GetSuccessors(this, null);
			}

			if (!ret)
			{
				m_Successors.Clear(); // empty vector of successor nodes to n

				// free up everything else we allocated
				FreeSolutionNodes();

				m_State = SearchState.OutOfMemory;
				return m_State;
			}

			// Now handle each successor to the current node ...
			Node successor = null;
			int successors_size = m_Successors.Count;
			for (int i = 0; i < successors_size; ++i)
			{
				successor = m_Successors[i];

				// 	The g value for this successor ...
				float newg = n.g + n.m_UserState.GetCost(successor.m_UserState);

				// Now we need to find whether the node is on the open or closed lists
				// If it is but the node that is already on them is better (lower g)
				// then we can forget about this successor

				// First linear search of open list to find node
				Node openlist_result = null;
				int openlist_size = m_OpenList.Count;
				bool foundOpenNode = false;
				for (int j = 0; j < openlist_size; ++j)
				{
					openlist_result = m_OpenList[j];
					if (openlist_result.m_UserState.IsSameState(successor.m_UserState))
					{
						foundOpenNode = true;
						break;					
					}
				}

				if (foundOpenNode)
				{
					// we found this state on open
					if (openlist_result.g <= newg)
					{
						// the one on Open is cheaper than this one
						continue;
					}
				}

				Node closedlist_result = null;
				int closedlist_size = m_ClosedList.Count;
				bool foundClosedNode = false;
				for (int k = 0; k < closedlist_size; ++k)
				{
					closedlist_result = m_ClosedList[k];
					if (closedlist_result.m_UserState.IsSameState(successor.m_UserState))
					{
						foundClosedNode = true;
						break;					
					}
				}

				if (foundClosedNode)
				{
					// we found this state on closed
					if (closedlist_result.g <= newg)
					{
						// the one on Closed is cheaper than this one
						continue;
					}
				}

				// This node is the best node so far with this particular state
				// so lets keep it and set up its AStar specific data ...
				successor.parent = n;
				successor.g = newg;
				successor.h = successor.m_UserState.GoalDistanceEstimate( m_Goal.m_UserState );
				successor.f = successor.g + successor.h;

				// Remove successor from closed if it was on it
				if (foundClosedNode)
				{
					// remove it from Closed
					m_ClosedList.Remove(closedlist_result);
				}

				// Update old version of this node
				if (foundOpenNode)
				{	   
			   		m_OpenList.Remove(openlist_result);
				}

				SortedAddToOpenList(successor);
			}

			// push n onto Closed, as we have expanded it now
			m_ClosedList.Add(n);

			if (m_ClosedList.Count > closedListHighWaterMark)
			{
				closedListHighWaterMark = m_ClosedList.Count;
			}
		} // end else (not goal so expand)

 		return m_State; // 'Succeeded' bool is false at this point. 
	}

	// User calls this to add a successor to a list of successors
	// when expanding the search frontier
	public bool AddSuccessor(MapSearchNode state)
	{
		Node node = AllocateNode();

		if (node != null)
		{
			node.m_UserState = state;
			m_Successors.Add(node);

			if (m_Successors.Count > successorListHighWaterMark)
			{
				successorListHighWaterMark = m_Successors.Count;
			}
			return true;
		}

		return false;
	}

	// Get start node
	public MapSearchNode GetSolutionStart()
	{
		m_CurrentSolutionNode = m_Start;
		
		if (m_Start != null)
		{
			return m_Start.m_UserState;
		}
		else
		{
			return null;
		}
	}

	// Get next node
	public MapSearchNode GetSolutionNext()
	{
		if (m_CurrentSolutionNode != null)
		{
			if (m_CurrentSolutionNode.child != null)
			{
				Node child = m_CurrentSolutionNode.child;
				m_CurrentSolutionNode = m_CurrentSolutionNode.child;
				return child.m_UserState;
			}
		}

		return null;
	}

	// Free the solution nodes
	// This is done to clean up all used Node memory when you are done with the
	// search
	public void FreeSolutionNodes()
	{
		m_OpenList.Clear();
		m_ClosedList.Clear();
		m_Successors.Clear();

		for (int i = 0; i < kPreallocatedNodes; ++i)
		{
			m_FixedSizeAllocator[i].Reinitialize();
		}
	}
}
