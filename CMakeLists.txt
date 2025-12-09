cmake_minimum_required(VERSION 3.20)
project(astar-algorithm-cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)
FetchContent_Declare(
    doctest
    URL https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(doctest)

add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${doctest_SOURCE_DIR})

# Header-only library interface
add_library(stlastar INTERFACE)
target_include_directories(stlastar INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(8puzzle 8puzzle.cpp)
target_link_libraries(8puzzle stlastar)

add_executable(findpath findpath.cpp)
target_link_libraries(findpath stlastar)

add_executable(minpathbucharest min_path_to_Bucharest.cpp)
target_link_libraries(minpathbucharest stlastar)

enable_testing()
add_executable(tests tests.cpp)

target_link_libraries(tests stlastar doctest)

# Register with CTest so you can run 'ctest' in the build folder
add_test(NAME tests COMMAND tests)
