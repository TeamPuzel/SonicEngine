cmake_minimum_required(VERSION 3.21)
project(CoreGame CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE CORE_GAME_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE CORE_GAME_HEADERS CONFIGURE_DEPENDS src/*.hpp)

find_package(SDL2 REQUIRED)

add_library(CoreGame STATIC ${CORE_GAME_SOURCES} ${CORE_GAME_HEADERS})
target_link_libraries(CoreGame PRIVATE SDL2::SDL2)
target_link_libraries(CoreGame PRIVATE Core::Core)

add_library(Core::Game ALIAS CoreGame)

target_include_directories(CoreGame PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
