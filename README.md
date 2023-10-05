## astar-algorithm

![Build Status](https://github.com/justinhj/astar-algorithm-cpp/workflows/C/C++%20CI/badge.svg)

### Summary

This code is an efficient implementation in C++ and C# of the A* algorithm, designed to be used from high performance realtime applications (video games) and with an optional fast memory allocation scheme.

It accompanies this A* tutorial:
https://www.heyes-jones.com/astar.php

The A* algorithm was described in the paper https://ieeexplore.ieee.org/document/4082128 by Hart, Nillson and Raphael. 

This repository is dedicated to the memory of [Nils Nilsson](https://ai.stanford.edu/~nilsson/) who sadly passed away in 2019.

### Star History

[![Star History Chart](https://api.star-history.com/svg?repos=justinhj/astar-algorithm-cpp&type=Date)](https://star-history.com/#justinhj/astar-algorithm-cpp&Date)

### Release notes

[v1.2](https://github.com/justinhj/astar-algorithm-cpp/releases/tag/v1.2) 
Breaking changes! C++ 11 is now the minimum required C++ standard complicance.
User is now required to provide a Hash function for their Node type. Thanks to a contribution from @btdubs the closed 
list is now an unordered_set and this greatly speeds up the execution time of the algorithm.
Check the included demo code for examples of the Hash implementation for various Node types.

[v1.1](https://github.com/justinhj/astar-algorithm-cpp/releases/tag/v1.1) 
Code cleanup and final version that does not require C++11

[v1.0](https://github.com/justinhj/astar-algorithm-cpp/releases/tag/v1.0) 
Initial release once API stable.

### License

This software is released under the MIT License, see license.txt

### Commercial Use

This software has been used in a number of AAA video games, which is an area of software that relies on efficiency and reliability. In addition it has been used in a number of academic and personal projects that require efficient search. Please 

Commercial users of the code are encouraged to make a donation to http://www.unicef.org/ if they find this project useful.

### Projects using this code

If you wish to be added to the list of known products/educational projects using the code please contact me.

* Gun, Activision
* Company of Heroes (various versions), Relic Entertainment
* Angel Engine, a game prototyping engine http://code.google.com/p/angel-engine/
* War of Sonria, a strategy war game on PSP and Playstation 3 by Playground Squad
* Lighthouses AI contest https://github.com/marcan/lighthouses_aicontest
* Self-Driving Car Engineer Nanodegree Program https://github.com/vanAken/CarND-Path-Planning-Project

### Compilation

Enter the cpp folder and run make

#### Introduction

This implementation is intended to be simple to read yet fairly
efficient. To build it you can compile, with any recent C++ compiler,
the following files :

For 8-puzzle solver

* 8puzzle.cpp
* stlastar.h
* optionally fsa.h

#### Command line 

8puzzle with no arguments runs with one of the boards in the cpp file, you can
select the one you want changing the conditional compiliation instructions. Or if you
prefer pass in a board on the command line using digits for the tile positions, where
zero is the space. The board runs from left to right, each row at a time:
	
    8puzzle 013824765

For path finder 
* findpath.cpp
* stlastar.h
* optionally fsa.h

pathfind has no arguments. You can edit the simple map in pathfind.cpp and the start 
and goal co-ordinates to experiement with the pathfinder.

#### Fixed size allocator

FSA is just a simple memory pool that uses a doubly linked list of available nodes in an array. This is 
a very efficient way to manage memory. It has no automatic resizing, so you must account for the fact it 
will use a fixed block of memory per instance and will report an error when memory is exhausted.

As mentioned briefly in the tutorial you can enable and disable the faster memory allocation. This allocates 
a fixed size block of memory, so you have to specify this size with the astar constructor. You need to enlarge 
it if you hit an out of memory assert during the search.

Compatibility notes:

This version of the code requires any standards compliant C++ using std C++11
