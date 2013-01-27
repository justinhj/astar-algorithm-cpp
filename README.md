astar-algorithm-cpp
===================

Implementation of the A* algorithm in C++

This code is part of the site Justin Heyes-Jones has been running since 2001, teaching folk about the A algorithm.

See http://www.heyes-jones.com/astar.html to learn more.

The code is designed to be easy to understand, easy to add your own runtime debugging monitoring the open and closed lists for example. Efficiency is considered, and the open list is implemented as a priority queue using STL.

Now on google hosting, the project is now open to the public to help me maintain different versions and fix bugs.

Using a templated class to allow polymorphism without the overhead of vtables. In addition a simplified and fast memory manager is included, speeding up the search.

Two canonical examples of A are provided. The 8-puzzle and a path finder.

You can email Justin at justinhj@gmail.com


