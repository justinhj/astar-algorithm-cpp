////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This example code illustrate how to use STL A* Search implementation to find the minimum path between two
// cities given a map. The example is taken from the book AI: A Modern Approach, 3rd Ed., by Russel, where a map
// of Romania is given. The target node is Bucharest, and the user can specify the initial city from which the
// search algorithm will start looking for the minimum path to Bucharest.
//
// Usage: min_path_to_Bucharest <start city name>
//        Example:
//                min_path_to_Bucharest Arad
//
// Rasoul Mojtahedzadeh
// Mojtahedzadeh _atsign_ gmail com
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stlastar.h"
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

#define DEBUG_LISTS 0
#define DEBUG_LIST_LENGTHS_ONLY 0

using namespace std;

const int MAX_CITIES = 20;

enum ENUM_CITIES{Arad=0, Bucharest, Craiova, Drobeta, Eforie, Fagaras, Giurgiu, Hirsova, Iasi, Lugoj, Mehadia, Neamt, Oradea, Pitesti, RimnicuVilcea, Sibiu, Timisoara, Urziceni, Vaslui, Zerind};
vector<string> CityNames(MAX_CITIES);
float RomaniaMap[MAX_CITIES][MAX_CITIES];

class PathSearchNode
{
public:

  ENUM_CITIES city;

	PathSearchNode() { city = Arad; }
	PathSearchNode( ENUM_CITIES in ) { city = in; }

	float GoalDistanceEstimate( PathSearchNode &nodeGoal );
	bool IsGoal( PathSearchNode &nodeGoal );
	bool GetSuccessors( AStarSearch<PathSearchNode> *astarsearch, PathSearchNode *parent_node );
	float GetCost( PathSearchNode &successor );
	bool IsSameState( PathSearchNode &rhs );

	void PrintNodeInfo();
};

// check if "this" node is the same as "rhs" node
bool PathSearchNode::IsSameState( PathSearchNode &rhs )
{
  if(city == rhs.city) return(true);
  return(false);
}

// Euclidean distance between "this" node city and Bucharest
// Note: Numbers are taken from the book
float PathSearchNode::GoalDistanceEstimate( PathSearchNode &nodeGoal )
{
  // goal is always Bucharest!
  switch(city)
  {
    case Arad: return 366; break;
    case Bucharest: return 0; break;
    case Craiova: return 160; break;
    case Drobeta: return 242; break;
    case Eforie: return 161; break;
    case Fagaras: return 176; break;
    case Giurgiu: return 77; break;
    case Hirsova: return 151; break;
    case Iasi: return 226; break;
    case Lugoj: return 244; break;
    case Mehadia: return 241; break;
    case Neamt: return 234; break;
    case Oradea: return 380; break;
    case Pitesti: return 100; break;
    case RimnicuVilcea: return 193; break;
    case Sibiu: return 253; break;
    case Timisoara: return 329; break;
    case Urziceni: return 80; break;
    case Vaslui: return 199; break;
    case Zerind: return 374; break;
  }
  cerr << "ASSERT: city = " << CityNames[city] << endl;
	return 0;
}

// check if "this" node is the goal node
bool PathSearchNode::IsGoal( PathSearchNode &nodeGoal )
{
	if( city == Bucharest ) return(true);
	return(false);
}

// generates the successor nodes of "this" node
bool PathSearchNode::GetSuccessors( AStarSearch<PathSearchNode> *astarsearch, PathSearchNode *parent_node )
{
  PathSearchNode NewNode;
  for(int c=0; c<MAX_CITIES; c++)
  {
    if(RomaniaMap[city][c] < 0) continue;
    NewNode = PathSearchNode((ENUM_CITIES)c);
    astarsearch->AddSuccessor( NewNode );
  }
	return true;
}

// the cost of going from "this" node to the "successor" node
float PathSearchNode::GetCost( PathSearchNode &successor )
{
	return RomaniaMap[city][successor.city];
}

// prints out information about the node
void PathSearchNode::PrintNodeInfo()
{
	cout << " " << CityNames[city] << "\n";
}

int main( int argc, char *argv[] )
{
  // creating map of Romania
  for(int i=0; i<MAX_CITIES; i++)
    for(int j=0; j<MAX_CITIES; j++)
      RomaniaMap[i][j]=-1.0;

  RomaniaMap[Arad][Sibiu]=140;
  RomaniaMap[Arad][Zerind]=75;
  RomaniaMap[Arad][Timisoara]=118;
  RomaniaMap[Bucharest][Giurgiu]=90;
  RomaniaMap[Bucharest][Urziceni]=85;
  RomaniaMap[Bucharest][Fagaras]=211;
  RomaniaMap[Bucharest][Pitesti]=101;
  RomaniaMap[Craiova][Drobeta]=120;
  RomaniaMap[Craiova][RimnicuVilcea]=146;
  RomaniaMap[Craiova][Pitesti]=138;
  RomaniaMap[Drobeta][Craiova]=120;
  RomaniaMap[Drobeta][Mehadia]=75;
  RomaniaMap[Eforie][Hirsova]=75;
  RomaniaMap[Fagaras][Bucharest]=211;
  RomaniaMap[Fagaras][Sibiu]=99;
  RomaniaMap[Giurgiu][Bucharest]=90;
  RomaniaMap[Hirsova][Eforie]=86;
  RomaniaMap[Hirsova][Urziceni]=98;
  RomaniaMap[Iasi][Vaslui]=92;
  RomaniaMap[Iasi][Neamt]=87;
  RomaniaMap[Lugoj][Timisoara]=111;
  RomaniaMap[Lugoj][Mehadia]=70;
  RomaniaMap[Mehadia][Lugoj]=70;
  RomaniaMap[Mehadia][Drobeta]=75;
  RomaniaMap[Neamt][Iasi]=87;
  RomaniaMap[Oradea][Zerind]=71;
  RomaniaMap[Oradea][Sibiu]=151;
  RomaniaMap[Pitesti][Bucharest]=101;
  RomaniaMap[Pitesti][RimnicuVilcea]=97;
  RomaniaMap[Pitesti][Craiova]=138;
  RomaniaMap[RimnicuVilcea][Pitesti]=97;
  RomaniaMap[RimnicuVilcea][Craiova]=146;
  RomaniaMap[RimnicuVilcea][Sibiu]=80;
  RomaniaMap[Sibiu][RimnicuVilcea]=80;
  RomaniaMap[Sibiu][Fagaras]=99;
  RomaniaMap[Sibiu][Oradea]=151;
  RomaniaMap[Sibiu][Arad]=140;
  RomaniaMap[Timisoara][Arad]=118;
  RomaniaMap[Timisoara][Lugoj]=111;
  RomaniaMap[Urziceni][Bucharest]=85;
  RomaniaMap[Urziceni][Hirsova]=98;
  RomaniaMap[Urziceni][Vaslui]=142;
  RomaniaMap[Vaslui][Urziceni]=142;
  RomaniaMap[Vaslui][Iasi]=92;
  RomaniaMap[Zerind][Arad]=75;
  RomaniaMap[Zerind][Oradea]=71;

  // City names
  CityNames[Arad].assign("Arad");
  CityNames[Bucharest].assign("Bucharest");
  CityNames[Craiova].assign("Craiova");
  CityNames[Drobeta].assign("Drobeta");
  CityNames[Eforie].assign("Eforie");
  CityNames[Fagaras].assign("Fagaras");
  CityNames[Giurgiu].assign("Giurgiu");
  CityNames[Hirsova].assign("Hirsova");
  CityNames[Iasi].assign("Iasi");
  CityNames[Lugoj].assign("Lugoj");
  CityNames[Mehadia].assign("Mehadia");
  CityNames[Neamt].assign("Neamt");
  CityNames[Oradea].assign("Oradea");
  CityNames[Pitesti].assign("Pitesti");
  CityNames[RimnicuVilcea].assign("RimnicuVilcea");
  CityNames[Sibiu].assign("Sibiu");
  CityNames[Timisoara].assign("Timisoara");
  CityNames[Urziceni].assign("Urziceni");
  CityNames[Vaslui].assign("Vaslui");
  CityNames[Zerind].assign("Zerind");

  ENUM_CITIES initCity = Arad;
  if(argc == 2)
  {
    bool found = false;
    for(size_t i=0; i<CityNames.size(); i++)
      if(CityNames[i].compare(argv[1])==0)
      {
        initCity = (ENUM_CITIES)i;
        found = true;
        break;
      }
    if(not found)
    {
      cout << "There is no city named "<<argv[1]<<" in the map!\n";
      return(1);
    }
  }

  // An instance of A* search class
	AStarSearch<PathSearchNode> astarsearch;

	unsigned int SearchCount = 0;
	const unsigned int NumSearches = 1;

	while(SearchCount < NumSearches)
	{
		// Create a start state
		PathSearchNode nodeStart;
		nodeStart.city = initCity;

		// Define the goal state, always Bucharest!
		PathSearchNode nodeEnd;
		nodeEnd.city = Bucharest;

		// Set Start and goal states
		astarsearch.SetStartAndGoalStates( nodeStart, nodeEnd );

		unsigned int SearchState;
		unsigned int SearchSteps = 0;

		do
		{
			SearchState = astarsearch.SearchStep();
			SearchSteps++;

	#if DEBUG_LISTS

			cout << "Steps:" << SearchSteps << "\n";

			int len = 0;

			cout << "Open:\n";
			PathSearchNode *p = astarsearch.GetOpenListStart();
			while( p )
			{
				len++;
	#if !DEBUG_LIST_LENGTHS_ONLY
				((PathSearchNode *)p)->PrintNodeInfo();
	#endif
				p = astarsearch.GetOpenListNext();

			}
			cout << "Open list has " << len << " nodes\n";

			len = 0;

			cout << "Closed:\n";
			p = astarsearch.GetClosedListStart();
			while( p )
			{
				len++;
	#if !DEBUG_LIST_LENGTHS_ONLY
				p->PrintNodeInfo();
	#endif
				p = astarsearch.GetClosedListNext();
			}

			cout << "Closed list has " << len << " nodes\n";
	#endif

		}
		while( SearchState == AStarSearch<PathSearchNode>::SEARCH_STATE_SEARCHING );

		if( SearchState == AStarSearch<PathSearchNode>::SEARCH_STATE_SUCCEEDED )
		{
			cout << "Search found the goal state\n";
      PathSearchNode *node = astarsearch.GetSolutionStart();
      cout << "Displaying solution\n";
      int steps = 0;
      node->PrintNodeInfo();
      for( ;; )
      {
        node = astarsearch.GetSolutionNext();
        if( !node ) break;
        node->PrintNodeInfo();
        steps ++;
      };
      cout << "Solution steps " << steps << endl;
      // Once you're done with the solution you can free the nodes up
			astarsearch.FreeSolutionNodes();
		}
		else if( SearchState == AStarSearch<PathSearchNode>::SEARCH_STATE_FAILED )
		{
			cout << "Search terminated. Did not find goal state\n";
		}
		// Display the number of loops the search went through
		cout << "SearchSteps : " << SearchSteps << "\n";
		SearchCount ++;
		astarsearch.EnsureMemoryFreed();
	}

	return 0;
}
