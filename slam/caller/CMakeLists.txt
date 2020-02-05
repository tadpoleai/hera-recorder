cmake_minimum_required(VERSION 3.5.1)
project(hera-slam-caller)

include("../../deploy/cmake/cpp_common.cmake")

include_directories(../.. include)

add_library(${PROJECT_NAME} STATIC src/caller.cpp)

add_executable(${PROJECT_NAME}-start test/start.cpp)
target_link_libraries(${PROJECT_NAME}-start ${PROJECT_NAME})

add_executable(${PROJECT_NAME}-stop test/stop.cpp)
target_link_libraries(${PROJECT_NAME}-stop ${PROJECT_NAME})

install(
  TARGETS ${PROJECT_NAME}-start
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION /usr/local/bin)

install(
  TARGETS ${PROJECT_NAME}-stop
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION /usr/local/bin)
