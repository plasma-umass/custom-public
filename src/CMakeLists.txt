cmake_minimum_required(VERSION 3.16)

include(FetchContent)

project(litterer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Threads REQUIRED)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG a33701196adfad74917046096bf5a2aa0ab0bb50 # 9.1.0
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG bc889afb4c5bf1c0d8ee29ef35eaaf4c8bef8a5d # v3.11.2
)

message(STATUS "Downloading and configuring {fmt}")
FetchContent_MakeAvailable(fmt)

message(STATUS "Downloading and configuring nlohmann::json")
FetchContent_MakeAvailable(nlohmann_json)

if(NOT WIN32)
add_library(detector SHARED detector.cpp)
target_include_directories(detector PRIVATE include)
target_link_libraries(detector PRIVATE ${CMAKE_DL_LIBS})
endif()

add_library(litterer_static STATIC litterer.cpp)
target_include_directories(litterer_static PUBLIC include)
target_link_libraries(litterer_static PRIVATE ${CMAKE_DL_LIBS})
target_link_libraries(litterer_static PRIVATE fmt)
target_link_libraries(litterer_static PRIVATE nlohmann_json)
target_link_libraries(litterer_static PRIVATE Threads::Threads)

add_library(litterer SHARED litterer-standalone.cpp)
target_link_libraries(litterer PRIVATE litterer_static)

if(MSVC)
    target_compile_options(litterer_static PRIVATE /W4 /WX)
    target_compile_options(litterer PRIVATE /W4 /WX)

    target_compile_definitions(litterer_static PRIVATE -D_CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(litterer PRIVATE -D_CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(detector PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(litterer_static PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(litterer PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
