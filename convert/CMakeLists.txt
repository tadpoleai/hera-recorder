cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-convert)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")
include("../deploy/cmake/ros.cmake")
include("../deploy/cmake/pcl_common.cmake")

# Find ROS Bag and Eigen3
set(CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/cmake;${ROS_ROOT_DIR}")
find_package(Threads REQUIRED)
find_package(rosbag REQUIRED)
find_package(Eigen3 REQUIRED)

file(GLOB_RECURSE ALL_SRC "src/*.cpp" "src/**/*.cpp")
include_directories("../" "/usr/local/include/" ${rosbag_INCLUDE_DIRS}
                    ${PCL_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIR})
add_executable(${PROJECT_NAME} ${ALL_SRC})
target_link_libraries(${PROJECT_NAME} hera-common hera-device
                      ${rosbag_LIBRARIES})

install(
  TARGETS hera-convert
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION /usr/local/bin)
