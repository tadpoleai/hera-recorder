cmake_minimum_required( VERSION 2.8 )

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(PROJECT_NAME sensor_collector)
project(${PROJECT_NAME})

find_package(Threads REQUIRED)

file(GLOB_RECURSE
    ALL_SRC
    "src/*.cpp"
    "src/**/*.cpp"
    "../common/**/*.cpp"
)
include_directories(
    "../"
    "/usr/local/include/"
)

#add_executable(dummy_test
#    test/dummy_test.cpp
#    ${ALL_SRC}
#)
#target_link_libraries(dummy_test
#    Threads::Threads
#)

add_executable(daemon
    ${ALL_SRC}
)
target_link_libraries(daemon
    Threads::Threads
    "/usr/local/lib/libthrift-0.12.0.so"
)