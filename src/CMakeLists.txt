cmake_minimum_required(VERSION 3.16)

include(FetchContent)

project(litterer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG 015d1a091af6937488242b70121858bce8fd40e9 # v1.8.2
)

FetchContent_Declare(
    mimalloc
    GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
    GIT_TAG 43ce4bd7fd34bcc730c1c7471c99995597415488 # v2.1.2
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG bc889afb4c5bf1c0d8ee29ef35eaaf4c8bef8a5d # v3.11.2
)

message(STATUS "Downloading and configuring benchmark")
set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
FetchContent_MakeAvailable(benchmark)

message(STATUS "Downloading and configuring nlohmann::json")
FetchContent_MakeAvailable(nlohmann_json)

add_library(litterer_static STATIC litterer.cpp)
target_include_directories(litterer_static PUBLIC include)
target_link_libraries(litterer_static PRIVATE ${CMAKE_DL_LIBS})
target_link_libraries(litterer_static PRIVATE nlohmann_json)
target_link_libraries(litterer_static PRIVATE Threads::Threads)

add_executable(microbenchmark-pages microbenchmark-pages.cpp)
target_link_libraries(microbenchmark-pages PRIVATE benchmark)

if(NOT WIN32)
    message(STATUS "Downloading and configuring mimalloc")
    set(MI_OVERRIDE OFF)
    FetchContent_MakeAvailable(mimalloc)

    add_library(detector SHARED detector.cpp)
    target_compile_options(detector PRIVATE -fno-builtin-malloc)
    target_include_directories(detector PRIVATE include)
    target_link_libraries(detector PRIVATE ${CMAKE_DL_LIBS})
    target_link_libraries(detector PRIVATE mimalloc-static)

    add_library(litterer SHARED litterer-standalone.cpp)
    target_link_libraries(litterer PRIVATE litterer_static)
else()
    add_executable(size-classes size-classes.cpp)
    target_link_libraries(size-classes PRIVATE Psapi)
endif()

if(MSVC)
    target_compile_options(litterer_static PRIVATE /W4 /WX)
    target_compile_definitions(litterer_static PRIVATE -D_CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(detector PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(litterer_static PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(litterer PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
