cmake_minimum_required(VERSION 3.5.1)
project(packages_to_laserscan_node)

include("../../deploy/cmake/cpp_common.cmake")
include("../../deploy/cmake/pcl_common.cmake")

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
  tf2
  tf2_ros
  eigen_conversions
)


include_directories(
    ../..
    /usr/local/include/
    ros/include
    ${PCL_INCLUDE_DIRS}
    ${catkin_INCLUDE_DIRS}
)

link_directories(
  ${PCL_LIBRARY_DIRS}
)
add_definitions(${PCL_DEFINITIONS})

message(STATUS "PCL_INCLUDE_DIRS:" ${PCL_INCLUDE_DIRS})
message(STATUS "PCL_LIBRARY_DIRS:" ${PCL_LIBRARY_DIRS})
message(STATUS "PCL_DEFINITIONS:" ${PCL_DEFINITIONS})

catkin_package(
  INCLUDE_DIRS ros/include
  # LIBRARIES packages_to_laserscan_node
  CATKIN_DEPENDS roscpp rospy std_msgs
  #  DEPENDS system_lib
)


set(distribution_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../build/slam)
message (${distribution_DIR})

add_executable(packages_to_laserscan_node ros/node/packages_to_laserscan_node.cpp)
add_executable(points2package_hera_node ros/node/points2package_hera_node.cpp)

add_library(package_accumulator_velodyne STATIC ros/src/package_accumulator_velodyne.cpp)
add_library(pointcloud2laserscan_velodyne STATIC ros/src/pointcloud2laserscan_velodyne.cpp)

target_link_libraries(package_accumulator_velodyne ${catkin_LIBRARIES} ${EIGEN3_LIBRARIES})
target_link_libraries(pointcloud2laserscan_velodyne ${catkin_LIBRARIES} )

target_link_libraries(packages_to_laserscan_node  package_accumulator_velodyne pointcloud2laserscan_velodyne ${catkin_LIBRARIES} ${EIGEN3_LIBRARIES} )
target_link_libraries(points2package_hera_node hera-common hera-device ${catkin_LIBRARIES})

# copy executable to build folder
add_custom_command(TARGET packages_to_laserscan_node
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:packages_to_laserscan_node> ${distribution_DIR})

add_custom_command(TARGET points2package_hera_node
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:points2package_hera_node> ${distribution_DIR})

message(${CMAKE_CURRENT_SOURCE_DIR}/ros/launch/)
message(${CMAKE_CURRENT_SOURCE_DIR}/../../build/devel/)

# copy launch & config file to "devel" folder
add_custom_command(TARGET packages_to_laserscan_node POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../../build/devel/launch
                   COMMAND "${CMAKE_COMMAND}" -E 
                   copy "${CMAKE_CURRENT_SOURCE_DIR}/ros/launch/packages_to_laserscan.launch"
                   "${CMAKE_CURRENT_SOURCE_DIR}/../../build/devel/launch/packages_to_laserscan.launch"
#                   **** the following 2 lines are for potential future debug purpose ****
#                   COMMAND "${CMAKE_COMMAND}" -E
#                   remove_directory "${CMAKE_CURRENT_BINARY_DIR}"
                   COMMENT "Copying launch to devel directory")
