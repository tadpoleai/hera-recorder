cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-replay)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")

find_package(Threads REQUIRED)

file(GLOB_RECURSE ALL_SRC "src/*.cpp" "src/**/*.cpp")
include_directories("../" "/usr/local/include/")
add_executable(${PROJECT_NAME} ${ALL_SRC})
target_link_libraries(${PROJECT_NAME} hera-common hera-device hera-storage)

install(
  TARGETS hera-replay
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION /usr/local/bin)
