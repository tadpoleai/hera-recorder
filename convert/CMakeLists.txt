cmake_minimum_required( VERSION 3.5.1 )

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(PROJECT_NAME convert)
project(${PROJECT_NAME})

set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/cmake")
set(ROS_ROOT_DIR "/opt/ros/kinetic")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${ROS_ROOT_DIR}")

find_package(Threads REQUIRED)
find_package(rosbag REQUIRED)
find_package(PCL REQUIRED)

file(GLOB_RECURSE
    ALL_SRC
    "src/*.cpp"
    "src/**/*.cpp"
)
include_directories(
    "../"
    "../rosbag_direct_write/include/"
    "/usr/local/include/"
    ${rosbag_INCLUDE_DIRS}
    ${PCL_INCLUDE_DIRS}
)
add_executable(${PROJECT_NAME}
    ${ALL_SRC}
)
target_link_libraries(${PROJECT_NAME}
    devices
    rosbag_direct_write
    ${rosbag_LIBRARIES}
    ${PCL_LIBRARIES}
)