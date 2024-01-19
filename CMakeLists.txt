cmake_minimum_required(VERSION 3.24)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

project(litterer)

FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
  GIT_TAG af442b4da0cd7a07b56fa709bd16571889dc7fda # v3.0
)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG 344117638c8ff7e239044fd0fa7085839fc03021 # v1.8.3
)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG e69e5f977d458f2650bb346dadf2ad30c5320281 # 10.2.1
)

FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)

message(STATUS "Downloading and configuring argparse")
FetchContent_MakeAvailable(argparse)

message(STATUS "Downloading and configuring Benchmark")
set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(benchmark)

message(STATUS "Downloading and configuring {fmt}")
set(BUILD_SHARED_LIBS_OLD ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS ON)
FetchContent_MakeAvailable(fmt)
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_OLD})

message(STATUS "Downloading and configuring nlohmann::json")
FetchContent_MakeAvailable(nlohmann_json)

if (MSVC)
 add_compile_options(/W4)
 add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

# Interposition is not supported on Windows, so detectors are not useful.
if(NOT WIN32)
  add_library(detector_blank SHARED src/blank/detector.cpp)
  target_link_libraries(detector_blank PRIVATE ${CMAKE_DL_LIBS})
endif()

add_library(litterer_distribution SHARED src/distribution/litterer.cpp)
target_link_libraries(litterer_distribution PRIVATE fmt nlohmann_json)
if(NOT WIN32)
  target_link_libraries(litterer_distribution PRIVATE ${CMAKE_DL_LIBS})
endif()

add_executable(benchmark_iterator src/benchmarks/iterator.cpp)
target_link_libraries(benchmark_iterator PRIVATE argparse benchmark)
