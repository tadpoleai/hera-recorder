cmake_minimum_required(VERSION 3.5.1)
project(hera-slam-result)

include("../../cmake/include/cpp_common.cmake")

include_directories(../.. include)

add_library(${PROJECT_NAME} STATIC src/result.cpp)
target_link_libraries(${PROJECT_NAME} hera-common)

add_executable(${PROJECT_NAME}-test test/test.cpp)
target_link_libraries(${PROJECT_NAME}-test ${PROJECT_NAME} hera-common)

install(
  TARGETS ${PROJECT_NAME}-test
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION bin)
