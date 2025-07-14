cmake_minimum_required(VERSION 3.22)

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
  GIT_TAG eddb0241389718a23a42db6af5f0164b6e0139af # v1.9.4
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

if(MSVC)
 add_compile_options(/W4)
 add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

add_library(litterer_distribution SHARED src/distribution/litterer.cpp)
install(TARGETS litterer_distribution)
target_include_directories(litterer_distribution PRIVATE include)
target_link_libraries(litterer_distribution PRIVATE fmt nlohmann_json)
if(NOT WIN32)
  target_link_libraries(litterer_distribution PRIVATE ${CMAKE_DL_LIBS})
endif()

if(LINUX)
  add_library(utils_perf SHARED src/utils/perf.cpp)
  target_include_directories(utils_perf PUBLIC include)
endif()

add_executable(benchmark_iterator src/benchmarks/iterator.cpp)
target_link_libraries(benchmark_iterator PRIVATE argparse)
target_link_libraries(benchmark_iterator PRIVATE benchmark::benchmark)
if(LINUX)
  target_link_libraries(benchmark_iterator PRIVATE utils_perf)
  target_compile_definitions(benchmark_iterator PRIVATE -DENABLE_PERF)
endif()

# Interposition is not supported on Windows.
if(NOT WIN32)
  add_library(detector_blank SHARED src/blank/detector.cpp)
  install(TARGETS detector_blank)
  target_include_directories(detector_blank PRIVATE include)
  if(UNIX)
    target_link_libraries(detector_blank PRIVATE ${CMAKE_DL_LIBS})
  endif()

  add_library(detector_distribution SHARED src/distribution/detector.cpp)
  install(TARGETS detector_distribution)
  target_include_directories(detector_distribution PRIVATE include)
  target_link_libraries(detector_distribution PRIVATE nlohmann_json)
  if(UNIX)
    target_link_libraries(detector_distribution PRIVATE ${CMAKE_DL_LIBS})
  endif()

  add_library(litterer_distribution_standalone SHARED src/distribution/standalone.cpp)
  install(TARGETS litterer_distribution_standalone)
  target_include_directories(litterer_distribution_standalone PRIVATE include)
  target_link_libraries(litterer_distribution_standalone PUBLIC litterer_distribution)
  target_link_libraries(litterer_distribution_standalone PRIVATE fmt)

  add_library(logger SHARED src/logger/logger.cpp)
  install(TARGETS logger)
  target_include_directories(logger PRIVATE include)
  if(UNIX)
    target_link_libraries(logger PRIVATE ${CMAKE_DL_LIBS})
  endif()

  add_library(sequence SHARED src/sequence/sequence.cpp)
  target_include_directories(sequence PRIVATE include)
  target_link_libraries(sequence PRIVATE fmt)
  if(UNIX)
    target_link_libraries(sequence PRIVATE ${CMAKE_DL_LIBS})
  endif()
endif()

add_executable(fragmentation src/logger/fragmentation.cpp)
target_link_libraries(fragmentation PRIVATE argparse)

add_executable(consecutive src/logger/consecutive.cpp)
target_link_libraries(consecutive PRIVATE argparse)
