astar-algorithm-cpp
===================

Summary
-------

This code is an efficient implementation in C++ and C# of the A* algorithm. It accompanies the A* tutorial on this site: http://www.heyes-jones.com/astar.html

Thank you @ScaryG for the C# port. This should work on Unity as well as the .net or mono platforms. 

Thank you to Rasoul for submitting the path to Bucharest sample

The CPP code has been used in several popular video games including THQ's "Company of Heroes", Activision's "Gun" as well as in the popular game prototyping engine Angel. http://code.google.com/p/angel-engine/

It has also used in many University assignments and projects.

License
=======

This software is released under the MIT License, see license.txt

Commercial Use
==============

This software has been used in several widely distributed video games, as well as industrial and academic projects. Should you decide to ship this code in a commercial product, or wish to show your appreciation, I ask that you contact me just to let me know. I would also greatly appreciate it if you could contribute to Unicef, a charity which helps children worldwide. http://www.unicef.org/

If you wish to be added to the list of known products using the code please contact me.

Using this code:

* Gun, Activision
* Company of Heroes and Company of Heroes Online, Relic Entertainment
* Angel Engine, a game prototyping engine http://code.google.com/p/angel-engine/

Compilation
===========

Enter the cpp folder and run make

Introduction
============

This implementation is intended to be simple to read yet fairly
efficient. To build it you can compile, with any recent C++ compiler,
the following files :

For 8-puzzle solver

* 8puzzle.cpp
* stlastar.h
* optionally fsa.h

Command line 

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

Fixed size allocator notes: As mentioned briefly in the tutorial you can enable and disable the
faster memory allocation. This allocates a fixed size block of memory, so you have to specify this size
with the astar constructor. You need to enlarge it if you hit an out of memory assert during the
search.

Compilation notes:

Microsoft Visual C++ : Confirmed working with version 8.0.50727 with some deprecation warnings
I'm going to leave the deprecation warnings in so that it still works cleanly with GCC. 
TODO Make a non-deprecated compliant version using compiler checking

Compiled with:
Apple LLVM version 6.0 (clang-600.0.51) (based on LLVM 3.5svn)

Please let me know if it doesn't work for you and I will try to help. I cannot help if you are using
an old compiler such as Turbo C++, since I update the code to meet Ansi Standard C++ as required.


Cheers!

Justin
