cmake_minimum_required(VERSION 3.20)
project(mychannel)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Enable debug symbols and disable optimizations for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/utils.cpp
    src/media_queue.cpp
    src/media_info.cpp
    src/streaming.cpp
    src/http_server.cpp
)

add_executable(mychannel ${SOURCES})

# Add threading support for httplib
find_package(Threads REQUIRED)
target_link_libraries(mychannel Threads::Threads)

# Ensure debug symbols are included
target_compile_options(mychannel PRIVATE 
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)
