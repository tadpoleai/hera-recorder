cmake_minimum_required(VERSION 3.5.1)
project(hera-slam-bridge)

include("../../deploy/cmake/cpp_common.cmake")
include("../../deploy/cmake/turbojpeg.cmake")

find_package(roscpp REQUIRED)
include_directories(../.. include ${roscpp_INCLUDE_DIRS}
                    ${JPEG_TURBO_INCLUDE_DIR})

add_executable(${PROJECT_NAME} src/bridge_node.cpp src/bridge.cpp)
target_link_libraries(${PROJECT_NAME} hera-common hera-device hera-slam-result
                      ${roscpp_LIBRARIES} ${JPEG_TURBO_LIBS})

install(
  TARGETS ${PROJECT_NAME}
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION /usr/local/bin)
