cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME wrapper)
project(${PROJECT_NAME})

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Use one of below

# find_package(HeraArm)

# include(FindHeraArm.cmake)

message(STATUS "Find Hera of ARM in ${HERA_INCLUDE_DIRS}")

include_directories(${HERA_INCLUDE_DIRS} include)

add_executable(${PROJECT_NAME} src/bridge.cpp src/main.cpp)

target_link_libraries(${PROJECT_NAME} ${HERA_LIBS})
