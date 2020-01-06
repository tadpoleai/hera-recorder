cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-slam)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")
include("../deploy/cmake/pcl_common.cmake")

include_directories("../" "/usr/local/include/")

add_executable(${PROJECT_NAME} src/read_test.cpp)
target_link_libraries(${PROJECT_NAME} hera-common hera-device)

install(
  TARGETS hera-slam
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION /usr/local/bin)
