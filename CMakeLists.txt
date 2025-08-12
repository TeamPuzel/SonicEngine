cmake_minimum_required(VERSION 3.21)
project(Sonic CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS
    "-Wall -g -Wno-unused -Wpedantic -Wno-logical-op-parentheses -flto"
)
set(CMAKE_CXX_FLAGS_DEBUG "-ftrivial-auto-var-init=pattern")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(SDL3 REQUIRED)

# Collect core engine sources.
file(GLOB_RECURSE SONIC_SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE SONIC_HEADERS CONFIGURE_DEPENDS src/*.hpp)
# Collect hot-reloadable object class sources.
file(GLOB_RECURSE SONIC_OBJECT_HEADERS CONFIGURE_DEPENDS object/*.hpp)
file(GLOB_RECURSE SONIC_OBJECT_SOURCES CONFIGURE_DEPENDS object/*.cpp)

# Build engine binary only if not hot reloading.
if(NOT HOT_RELOAD)
    add_executable(sonic ${SONIC_SOURCES} ${SONIC_HEADERS})
    target_link_libraries(sonic PRIVATE SDL3::SDL3)
    target_include_directories(sonic PRIVATE include)
endif()

# Build each object source file as its own shared library.
foreach(obj_src ${SONIC_OBJECT_SOURCES})
    get_filename_component(obj_name ${obj_src} NAME_WE)

    add_library(${obj_name} SHARED ${obj_src})
    target_link_libraries(${obj_name} PRIVATE SDL3::SDL3)
    target_include_directories(${obj_name} PRIVATE include)

    # Set output directory for shared objects, also for Windows dll files.
    set_target_properties(${obj_name} PROPERTIES
        PREFIX ""
        SUFFIX ".object"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/obj"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/obj"
    )
endforeach()

# Handle resource inclusion into the bundle, currently uncompressed.
file(GLOB_RECURSE RESOURCES res/*)

foreach(RESOURCE ${RESOURCES})
    file(COPY ${RESOURCE} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/res")
endforeach()
