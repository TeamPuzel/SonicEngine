cmake_minimum_required(VERSION 3.21)
project(Sonic CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(mod/Core)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

file(GLOB_RECURSE SONIC_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE SONIC_HEADERS CONFIGURE_DEPENDS src/*.hpp)

add_executable(sonic ${SONIC_SOURCES} ${SONIC_HEADERS})
target_link_libraries(sonic PRIVATE SDL2::SDL2)
target_link_libraries(sonic PRIVATE OpenGL::GL)
target_link_libraries(sonic PRIVATE Core::Core)
