cmake_minimum_required(VERSION 3.5.1)
project(packages_to_laserscan_node)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++11)

set(CATKIN_DEVEL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../../build/slam/hera-slam")

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
  std_msgs
  sensor_msgs
  nav_msgs
  geometry_msgs
  nodelet
  pcl_ros
  pcl_conversions
  tf
  tf_conversions
  eigen_conversions
)

find_package(PCL 1.7 REQUIRED)
include_directories(
    # ./include
    ${PCL_INCLUDE_DIRS}
    ${catkin_INCLUDE_DIRS}
)

link_directories(
  ${PCL_LIBRARY_DIRS}
  # /usr/lib
)
add_definitions(${PCL_DEFINITIONS})

message(STATUS "PCL_INCLUDE_DIRS:" ${PCL_INCLUDE_DIRS})
message(STATUS "PCL_LIBRARY_DIRS:" ${PCL_LIBRARY_DIRS})
message(STATUS "PCL_DEFINITIONS:" ${PCL_DEFINITIONS})

catkin_package(
  INCLUDE_DIRS ros
  # LIBRARIES packages_to_laserscan_node
  CATKIN_DEPENDS roscpp rospy std_msgs
  #  DEPENDS system_lib
)

add_executable(packages_to_laserscan_node ros/packages_to_laserscan_node.cpp)

add_library(velodyne_accumulator SHARED ros/velodyne_accumulator.cpp)

target_link_libraries(velodyne_accumulator ${catkin_LIBRARIES} ${EIGEN3_LIBRARIES})

target_link_libraries(packages_to_laserscan_node  velodyne_accumulator ${catkin_LIBRARIES} ${EIGEN3_LIBRARIES} )
