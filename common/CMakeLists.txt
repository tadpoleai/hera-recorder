cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-common)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")

find_package(Threads REQUIRED)

file(GLOB_RECURSE ALL_SRC "**/*.cpp")
include_directories("../" "/usr/local/include/")
add_library(${PROJECT_NAME} STATIC ${ALL_SRC})
target_link_libraries(${PROJECT_NAME} Threads::Threads)
