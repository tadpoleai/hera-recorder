cmake_minimum_required( VERSION 2.8 )

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_BUILD_TYPE "Release")

set(PROJECT_NAME sensor_collector)
project(${PROJECT_NAME})

find_package(Threads REQUIRED)

file(GLOB_RECURSE
    ALL_SRC
    "src/*.cpp"
    "../../common/*.cpp"
)
include_directories(
    "../"
)
add_executable(sensor_test
    test/sensor_test.cpp
    ${ALL_SRC}
)
target_link_libraries(sensor_test
    Threads::Threads
)
