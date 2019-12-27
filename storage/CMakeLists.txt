cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-storage)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")

find_package(Threads REQUIRED)

file(GLOB_RECURSE ALL_SRC "src/*.cpp" "src/**/*.cpp")

include_directories("../")
add_library(${PROJECT_NAME} STATIC ${ALL_SRC})
target_link_libraries(${PROJECT_NAME} hera-common hera-device Threads::Threads)
