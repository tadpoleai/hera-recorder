cmake_minimum_required( VERSION 3.5.1 )

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(PROJECT_NAME common)
project(${PROJECT_NAME})

find_package(Threads REQUIRED)

add_custom_command(
    OUTPUT rpc/tron_constants.cpp
    OUTPUT rpc/tron_constants.h
    OUTPUT rpc/tron_types.cpp
    OUTPUT rpc/tron_type.h
    OUTPUT rpc/TronService.cpp
    OUTPUT rpc/TronService.h
    DEPENDS rpc/tron.thrift
    COMMAND "mkdir" "-p" "rpc"
    COMMAND "thrift" "-r" "--strict" "-out" "rpc" "--gen" "cpp" "${CMAKE_CURRENT_SOURCE_DIR}/rpc/tron.thrift"
)

file(GLOB_RECURSE
    ALL_SRC
    "**/*.cpp"
)
include_directories(
    "/usr/local/include/"
)
add_library(common
    ${ALL_SRC}
    rpc/tron_constants.cpp
    rpc/tron_types.cpp
    rpc/TronService.cpp
)
target_link_libraries(common
    Threads::Threads
    "/usr/local/lib/libthrift.so"
)