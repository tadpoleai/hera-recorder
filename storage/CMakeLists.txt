cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-storage)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")

find_package(Threads REQUIRED)
include_directories(include .. ${CMAKE_BINARY_DIR})

file(GLOB_RECURSE ALL_SRC "src/*.cpp" "src/**/*.cpp")
add_library(${PROJECT_NAME} SHARED ${ALL_SRC})
target_link_libraries(${PROJECT_NAME} hera-common Threads::Threads)

file(GLOB_RECURSE TOOL_SRC "tool/*.cpp")
add_executable(${PROJECT_NAME}-tool ${TOOL_SRC})
target_link_libraries(${PROJECT_NAME}-tool hera-common hera-device hera-storage)

# Install shared libraries
install(
  TARGETS ${PROJECT_NAME}
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION lib)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include/hera/storage/)

# Install tools
install(
  TARGETS ${PROJECT_NAME}-tool
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION bin)
