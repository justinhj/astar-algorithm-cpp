all : 8puzzle findpath minpathbucharest tests 15puzzle

clean :
	rm 8puzzle findpath minpathbucharest tests 15puzzle

minpathbucharest : min_path_to_Bucharest.cpp stlastar.h
	g++ -Wall min_path_to_Bucharest.cpp -o minpathbucharest

8puzzle : 8puzzle.cpp stlastar.h
	g++ -Wall 8puzzle.cpp -o 8puzzle

15puzzle : 15puzzle.cpp stlastar.h
	g++ -Wall 15puzzle.cpp -o 15puzzle

findpath : findpath.cpp stlastar.h
	g++ -Wall findpath.cpp -o findpath

tests : tests.cpp 8puzzle findpath minpathbucharest
	g++ -Wall tests.cpp -o tests

test: tests
	./tests






