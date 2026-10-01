cmake_minimum_required(VERSION 3.20)
project(astar-algorithm-cpp)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)
FetchContent_Declare(
    doctest
    URL https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h
    URL_HASH SHA256=44faa038e9c3f9728efbda143748d01124ea0a27f4bf78f35a15d8fab2e039fb
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

add_executable(bench bench.cpp)
target_link_libraries(bench stlastar)

# Historical benchmark targets
function(add_historical_bench target_name header_dir)
    add_executable(${target_name} bench.cpp)
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/${header_dir})
endfunction()

add_historical_bench(bench_baseline bench/64bdb20_baseline)
add_historical_bench(bench_step1 bench/7424a4c_step1)
add_historical_bench(bench_step2 bench/3ee685b_step2)
add_historical_bench(bench_step3 bench/a37013f_step3)

add_custom_target(bench_all DEPENDS bench bench_baseline bench_step1 bench_step2 bench_step3)

enable_testing()
add_executable(tests tests.cpp)

target_link_libraries(tests stlastar doctest)

# Register with CTest so you can run 'ctest' in the build folder
add_test(NAME tests COMMAND tests)
