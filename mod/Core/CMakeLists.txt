cmake_minimum_required(VERSION 3.21)
project(Core CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS src/*.hpp)

add_library(Core STATIC ${CORE_SOURCES} ${CORE_HEADERS})

add_library(Core::Core ALIAS Core)

target_include_directories(Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
