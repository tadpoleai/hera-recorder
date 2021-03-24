cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-convert)
project(${PROJECT_NAME})

include("../cmake/include/cpp_common.cmake")
include("../cmake/include/ros.cmake")
include("../cmake/include/pcl_common.cmake")

# Find ROS Bag and Eigen3
set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/cmake;${ROS_ROOT_DIR}")
find_package(Threads REQUIRED)
find_package(rosbag REQUIRED)
find_package(Eigen3 REQUIRED)

include_directories(.. src include ${rosbag_INCLUDE_DIRS} ${PCL_INCLUDE_DIRS}
                    ${EIGEN3_INCLUDE_DIR})

# ROS Message Library
set(ROS_MESSAGE_LIB ${PROJECT_NAME}-ros-message)
file(GLOB_RECURSE ROS_MESSAGE_SRC "src/ros_message_impl/*.cpp")
add_library(
  ${ROS_MESSAGE_LIB} SHARED "src/ros_message.cpp"
                            "src/direct_bag/direct_bag.cpp" ${ROS_MESSAGE_SRC})
target_link_libraries(${ROS_MESSAGE_LIB} ${rosbag_LIBRARIES} hera-common
                      hera-device)

# Main executable
add_executable(${PROJECT_NAME} "src/converter.cpp" "src/main.cpp")
target_link_libraries(${PROJECT_NAME} ${ROS_MESSAGE_LIB})

# Install shared libraries
install(
  TARGETS ${ROS_MESSAGE_LIB}
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION lib)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include/hera/convert/)

# Install executable
install(
  TARGETS hera-convert
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION bin)
