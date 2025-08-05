cmake_minimum_required(VERSION 3.21)
project(Sonic CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "-Wall -g -Wno-unused -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(SDL3 REQUIRED)

file(GLOB_RECURSE SONIC_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE SONIC_HEADERS CONFIGURE_DEPENDS src/*.hpp)

add_executable(sonic ${SONIC_SOURCES} ${SONIC_HEADERS})
target_link_libraries(sonic PRIVATE SDL3::SDL3)
target_include_directories(sonic PRIVATE src/include)

file(GLOB_RECURSE RESOURCES res/*)

foreach(RESOURCE ${RESOURCES})
    file(COPY ${RESOURCE} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/res")
endforeach()
