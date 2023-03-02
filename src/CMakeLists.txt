cmake_minimum_required(VERSION 3.16)

include(FetchContent)

project(litterer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

add_library(detector SHARED detector.cpp)
target_link_libraries(detector ${CMAKE_DL_LIBS})
set_property(TARGET detector PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET detector PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET detector PROPERTY VISIBILITY_INLINES_HIDDEN ON)

add_library(litterer SHARED litterer.cpp)
target_link_libraries(detector ${CMAKE_DL_LIBS})
set_property(TARGET litterer PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET litterer PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET litterer PROPERTY VISIBILITY_INLINES_HIDDEN ON)

add_library(litterer-static STATIC litterer.cpp)
target_link_libraries(detector ${CMAKE_DL_LIBS})
set_property(TARGET litterer-static PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET litterer-static PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET litterer-static PROPERTY VISIBILITY_INLINES_HIDDEN ON)

