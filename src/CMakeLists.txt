cmake_minimum_required(VERSION 3.16)

include(FetchContent)

project(litterer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        bc889afb4c5bf1c0d8ee29ef35eaaf4c8bef8a5d # v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

find_package(Threads REQUIRED)

add_library(detector SHARED detector.cpp)
target_include_directories(detector PRIVATE include)
target_link_libraries(detector PRIVATE ${CMAKE_DL_LIBS})
set_property(TARGET detector PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET detector PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET detector PROPERTY VISIBILITY_INLINES_HIDDEN ON)

add_library(litterer-static STATIC litterer.cpp)
target_include_directories(litterer-static PUBLIC include)
target_link_libraries(litterer-static PRIVATE nlohmann_json Threads::Threads ${CMAKE_DL_LIBS})
set_property(TARGET litterer-static PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET litterer-static PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET litterer-static PROPERTY VISIBILITY_INLINES_HIDDEN ON)

add_library(litterer SHARED litterer-standalone.cpp)
target_link_libraries(litterer PRIVATE litterer-static)
set_property(TARGET litterer PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET litterer PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET litterer PROPERTY VISIBILITY_INLINES_HIDDEN ON)

