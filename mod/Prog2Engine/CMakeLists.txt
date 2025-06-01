cmake_minimum_required(VERSION 3.21)
project(Prog2 CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE PROG2_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE PROG2_HEADERS CONFIGURE_DEPENDS src/*.h)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_library(Prog2 STATIC ${PROG2_SOURCES} ${PROG2_HEADERS})
target_link_libraries(Prog2 PRIVATE SDL2::SDL2)
target_link_libraries(Prog2 PRIVATE SDL2_image::SDL2_image)
target_link_libraries(Prog2 PRIVATE SDL2_mixer::SDL2_mixer)
target_link_libraries(Prog2 PRIVATE SDL2_ttf::SDL2_ttf)
target_link_libraries(Prog2 PRIVATE OpenGL::GL)
target_link_libraries(Prog2 PRIVATE Core::Core)

add_library(Prog2::Prog2 ALIAS Prog2)

target_include_directories(Prog2 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
