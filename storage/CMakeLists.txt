cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-storage)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")

find_package(Threads REQUIRED)
include_directories(include .. ${CMAKE_BINARY_DIR})

file(GLOB_RECURSE ALL_SRC "src/*.cpp" "src/**/*.cpp")

if(BUILD_STATIC)
  add_library(${PROJECT_NAME}-static STATIC ${ALL_SRC})
  target_link_libraries(${PROJECT_NAME}-static hera-common-static
                        hera-device-static Threads::Threads)
endif()

add_library(${PROJECT_NAME} SHARED ${ALL_SRC})
target_link_libraries(${PROJECT_NAME} hera-common Threads::Threads)

if(GIT_INFO_ENABLED)
  target_link_libraries(${PROJECT_NAME} hera-git-info)
  if(BUILD_STATIC)
    target_link_libraries(${PROJECT_NAME}-static hera-git-info)
  endif()
endif()

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
