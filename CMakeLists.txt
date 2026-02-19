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
  GIT_TAG e424e3f2e607da02742f73db84873b8084fc714c # 12.0.0
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
set(CMAKE_POSITION_INDEPENDENT_CODE_OLD ${CMAKE_POSITION_INDEPENDENT_CODE})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
FetchContent_MakeAvailable(fmt)
set(CMAKE_POSITION_INDEPENDENT_CODE ${CMAKE_POSITION_INDEPENDENT_CODE_OLD})

message(STATUS "Downloading and configuring nlohmann::json")
FetchContent_MakeAvailable(nlohmann_json)

if(MSVC)
 add_compile_options(/W4)
 add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)

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
  target_link_libraries(litterer_distribution_standalone PRIVATE fmt)
  target_link_libraries(litterer_distribution_standalone PRIVATE nlohmann_json)
  target_link_libraries(litterer_distribution_standalone PRIVATE ${CMAKE_DL_LIBS})

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

  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)

  add_library(pthread_crash SHARED src/utils/pthread_crash.c)
  target_link_libraries(pthread_crash PRIVATE Threads::Threads)
endif()

add_executable(fragmentation src/logger/fragmentation.cpp)
target_link_libraries(fragmentation PRIVATE argparse)

add_executable(consecutive src/logger/consecutive.cpp)
target_link_libraries(consecutive PRIVATE argparse)
