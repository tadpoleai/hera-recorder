cmake_minimum_required(VERSION 3.5.1)

include("../deploy/cmake/ros.cmake")

set(SLAM_CARTO_ROOT "/usr/local/share/hera/slam/carto/")
add_definitions(-DSLAM_CARTO_ROOT="${SLAM_CARTO_ROOT}")
add_definitions(-DSLAM_ROS_ROOT="${ROS_ROOT_DIR}")

add_subdirectory(carto)
add_subdirectory(bridge)
add_subdirectory(caller)
