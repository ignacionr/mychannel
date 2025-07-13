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
    src/mcp_server.cpp
)

add_executable(mychannel ${SOURCES})

# Add threading support for httplib
find_package(Threads REQUIRED)
target_link_libraries(mychannel Threads::Threads)

# Find Google Test
find_package(GTest REQUIRED)
enable_testing()

# Ensure debug symbols are included
target_compile_options(mychannel PRIVATE 
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O3>
)

# Test sources (excluding main.cpp)
set(TEST_SOURCES
    src/utils.cpp
    src/media_queue.cpp
    src/media_info.cpp
    src/streaming.cpp
    src/http_server.cpp
    src/mcp_server.cpp
)

# Create test executable
add_executable(mychannel_tests
    tests/test_mcp_json_parsing.cpp
    tests/test_mcp_tools_call.cpp
    tests/test_mcp_debug.cpp
    tests/test_main.cpp
    ${TEST_SOURCES}
)

target_link_libraries(mychannel_tests 
    Threads::Threads 
    GTest::gtest 
    GTest::gtest_main
)

# Add test
add_test(NAME MCPJsonParsingTests COMMAND mychannel_tests)
