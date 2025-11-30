cmake_minimum_required(VERSION 3.10)
project(astar-algorithm-cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Header-only library interface (optional but good practice)
add_library(stlastar INTERFACE)
target_include_directories(stlastar INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Executables
add_executable(8puzzle 8puzzle.cpp)
target_link_libraries(8puzzle stlastar)

add_executable(findpath findpath.cpp)
target_link_libraries(findpath stlastar)

add_executable(minpathbucharest min_path_to_Bucharest.cpp)
target_link_libraries(minpathbucharest stlastar)

# Tests
enable_testing()
add_executable(tests tests.cpp)
target_link_libraries(tests stlastar)
add_test(NAME tests COMMAND tests)
