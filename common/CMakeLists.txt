cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-common)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")

find_package(Threads REQUIRED)

file(GLOB_RECURSE ALL_SRC "src/*.cpp")
include_directories(".." include)
add_library(${PROJECT_NAME} SHARED ${ALL_SRC})
target_link_libraries(${PROJECT_NAME} Threads::Threads)

if(GIT_INFO_ENABLED)
  target_link_libraries(${PROJECT_NAME} hera-common-git-info)
endif()

# Install shared libraries
install(
  TARGETS ${PROJECT_NAME}
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION lib)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include/hera/common/)
