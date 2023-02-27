cmake_minimum_required(VERSION 3.16)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(litterer)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        a33701196adfad74917046096bf5a2aa0ab0bb50 # 9.1.0
)
FetchContent_MakeAvailable(fmt)

add_library(litterer_blank SHARED src/blank/litterer.cpp)
target_link_libraries(litterer_blank ${CMAKE_DL_LIBS})
set_property(TARGET litterer_blank PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET litterer_blank PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET litterer_blank PROPERTY VISIBILITY_INLINES_HIDDEN ON)

add_library(litterer_control SHARED src/control/litterer.cpp)
target_link_libraries(litterer_control fmt ${CMAKE_DL_LIBS})
set_property(TARGET litterer_control PROPERTY C_VISIBILITY_PRESET hidden)
set_property(TARGET litterer_control PROPERTY CXX_VISIBILITY_PRESET hidden)
set_property(TARGET litterer_control PROPERTY VISIBILITY_INLINES_HIDDEN ON)
