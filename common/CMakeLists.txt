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
    OUTPUT ${RPC_GEN_CPP}/tron_constants.cpp
    OUTPUT ${RPC_GEN_CPP}/tron_constants.h
    OUTPUT ${RPC_GEN_CPP}/tron_types.cpp
    OUTPUT ${RPC_GEN_CPP}/tron_type.h
    OUTPUT ${RPC_GEN_CPP}/TronService.cpp
    OUTPUT ${RPC_GEN_CPP}/TronService.h
    DEPENDS rpc/tron.thrift
    COMMAND "mkdir" "-p" "${RPC_GEN_CPP}"
    COMMAND "thrift" "-r" "--strict" "-out" "${RPC_GEN_CPP}" "--gen" "cpp" "${CMAKE_CURRENT_SOURCE_DIR}/rpc/tron.thrift"
 )

file(GLOB_RECURSE
    ALL_SRC
    "**/*.cpp"
)
include_directories(
    "/usr/local/include/"
)
add_library(${PROJECT_NAME}
    STATIC
    ${ALL_SRC}
    ${RPC_GEN_CPP}/tron_constants.cpp
    ${RPC_GEN_CPP}/tron_types.cpp
    ${RPC_GEN_CPP}/TronService.cpp
)
target_link_libraries(${PROJECT_NAME}
    "/usr/local/lib/libthrift.so"
)
