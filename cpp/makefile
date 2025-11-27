all : 8puzzle findpath minpathbucharest tests

# Set the C++ compiler to g++
CXX = g++

# Set some common C++ flags
CXXFLAGS = -std=c++11 -Wall

clean :
	rm 8puzzle findpath minpathbucharest tests

minpathbucharest : min_path_to_Bucharest.cpp stlastar.h
	$(CXX) $(CXXFLAGS) min_path_to_Bucharest.cpp -o minpathbucharest

8puzzle : 8puzzle.cpp stlastar.h
	$(CXX) $(CXXFLAGS) -Wall 8puzzle.cpp -o 8puzzle

findpath : findpath.cpp stlastar.h
	$(CXX) $(CXXFLAGS) findpath.cpp -o findpath

tests : tests.cpp 8puzzle findpath minpathbucharest
	$(CXX) $(CXXFLAGS) tests.cpp -o tests

test: tests
	./tests
