cmake_minimum_required(VERSION 3.16)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

project(litterer)

FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG af442b4da0cd7a07b56fa709bd16571889dc7fda # v3.0
)

FetchContent_Declare(
    google_benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG 344117638c8ff7e239044fd0fa7085839fc03021 # v1.8.3
)

message(STATUS "Downloading and configuring argparse")
FetchContent_MakeAvailable(argparse)

message(STATUS "Downloading and configuring Benchmark")
set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(google_benchmark)

add_library(litterer_blank SHARED src/blank/litterer.cpp)
target_link_libraries(litterer_blank PRIVATE ${CMAKE_DL_LIBS})

add_executable(benchmark_iterator src/benchmarks/iterator.cpp) 
target_link_libraries(benchmark_iterator PRIVATE argparse benchmark)
