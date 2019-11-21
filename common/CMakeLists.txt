cmake_minimum_required( VERSION 3.5.1 )

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(PROJECT_NAME common)
project(${PROJECT_NAME})

set(RPC_GEN_CPP "rpc/gen-cpp")
add_custom_command(
    OUTPUT ${RPC_GEN_CPP}/hera_types.cpp
    OUTPUT ${RPC_GEN_CPP}/hera_constants.cpp
    OUTPUT ${RPC_GEN_CPP}/AcquisitionManager.cpp
    DEPENDS rpc/hera.thrift
    COMMAND "mkdir" "-p" "${RPC_GEN_CPP}"
    COMMAND "thrift" "-out" "${RPC_GEN_CPP}"
        "--gen" "cpp:no_skeleton"
        "${CMAKE_CURRENT_SOURCE_DIR}/rpc/hera.thrift"
)

file(GLOB_RECURSE
    ALL_SRC
    "**/*.cpp"
)
include_directories(
    "../"
    "/usr/local/include/"
)
add_library(${PROJECT_NAME}
    STATIC
    ${ALL_SRC}
    ${RPC_GEN_CPP}/hera_types.cpp
    ${RPC_GEN_CPP}/hera_constants.cpp
    ${RPC_GEN_CPP}/AcquisitionManager.cpp
)
target_link_libraries(${PROJECT_NAME}
    "/usr/local/lib/libthrift.so"
)
