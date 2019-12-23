cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-daemon)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")
include("../deploy/cmake/thrift.cmake")

# Compile hera.thrift
set(RPC_GEN_CPP "${CMAKE_CURRENT_SOURCE_DIR}/rpc/gen-cpp")
add_custom_command(
  OUTPUT ${RPC_GEN_CPP}/hera_types.cpp
  OUTPUT ${RPC_GEN_CPP}/hera_constants.cpp
  OUTPUT ${RPC_GEN_CPP}/AcquisitionManager.cpp
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rpc/hera.thrift"
  COMMAND "mkdir" "-p" "${RPC_GEN_CPP}"
  COMMAND "thrift" "-out" "${RPC_GEN_CPP}" "--gen" "cpp:no_skeleton"
          "${CMAKE_CURRENT_SOURCE_DIR}/rpc/hera.thrift")

# Build daemon
file(GLOB_RECURSE ALL_SRC "src/*.cpp" "src/**/*.cpp")
include_directories("../" "/usr/local/include/" ${CMAKE_BINARY_DIR})
add_executable(
  ${PROJECT_NAME}
  ${ALL_SRC} ${RPC_GEN_CPP}/hera_types.cpp ${RPC_GEN_CPP}/hera_constants.cpp
  ${RPC_GEN_CPP}/AcquisitionManager.cpp)
target_link_libraries(${PROJECT_NAME} hera-common hera-devices ${THRIFT})

install(
  FILES script/hera-daemon
  PERMISSIONS OWNER_EXECUTE OWNER_READ
  DESTINATION /etc/init.d)
install(
  FILES script/hera-daemon.service
  PERMISSIONS OWNER_EXECUTE OWNER_READ
  DESTINATION /lib/systemd/system)
install(
  TARGETS hera-daemon
  PERMISSIONS OWNER_EXECUTE OWNER_READ
  DESTINATION /usr/local/bin)
