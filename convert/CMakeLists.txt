cmake_minimum_required( VERSION 3.5.1 )

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(PROJECT_NAME convert)
project(${PROJECT_NAME})

find_package(Threads REQUIRED)

file(GLOB_RECURSE
    ALL_SRC
    "**/*.cpp"
)
include_directories(
    "/usr/local/include/"
)
add_library(common
    ${ALL_SRC}
    ${RPC_GEN_CPP}/tron_constants.cpp
    ${RPC_GEN_CPP}/tron_types.cpp
    ${RPC_GEN_CPP}/TronService.cpp
)
target_link_libraries(common
    Threads::Threads
    "/usr/local/lib/libthrift.so"
)