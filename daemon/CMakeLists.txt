cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-daemon)
project(${PROJECT_NAME})

include("../cmake/include/cpp_common.cmake")
include("../cmake/include/thrift.cmake")
include("../cmake/include/libconfig.cmake")

include_directories(.. . include ${THRIFT_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})

# Build broadcast
add_library(
  ${PROJECT_NAME}-broadcast STATIC broadcast/message.cpp broadcast/listener.cpp
                                   broadcast/broadcaster.cpp)
target_link_libraries(${PROJECT_NAME}-broadcast hera-common)

# Build network
add_library(
  ${PROJECT_NAME}-network STATIC network/network.cpp)
target_link_libraries(${PROJECT_NAME}-network hera-common)

# Compile hera.thrift
set(RPC_GEN_CPP "${CMAKE_CURRENT_SOURCE_DIR}/rpc/gen-cpp")
add_custom_command(
  OUTPUT ${RPC_GEN_CPP}/hera_types.cpp
  OUTPUT ${RPC_GEN_CPP}/hera_types.h
  OUTPUT ${RPC_GEN_CPP}/hera_constants.cpp
  OUTPUT ${RPC_GEN_CPP}/hera_constants.h
  OUTPUT ${RPC_GEN_CPP}/Service.cpp
  OUTPUT ${RPC_GEN_CPP}/Service.h
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/rpc/hera.thrift"
  COMMAND "mkdir" "-p" "${RPC_GEN_CPP}"
  COMMAND "thrift" "-out" "${RPC_GEN_CPP}" "--gen" "cpp:no_skeleton"
          "${CMAKE_CURRENT_SOURCE_DIR}/rpc/hera.thrift")

# Build daemon
file(GLOB_RECURSE ALL_SRC "src/*.cpp" "src/**/*.cpp")
add_executable(
  ${PROJECT_NAME} ${ALL_SRC} ${RPC_GEN_CPP}/hera_types.cpp
                  ${RPC_GEN_CPP}/hera_constants.cpp ${RPC_GEN_CPP}/Service.cpp)
target_link_libraries(
  ${PROJECT_NAME}
  hera-common
  hera-storage
  hera-device
  ${PROJECT_NAME}-broadcast
  ${PROJECT_NAME}-network
  ${THRIFT}
  ${LIBCONFIG}
  hera-git-info)

# Build finder
add_executable(${PROJECT_NAME}-finder "finder/finder.cpp")
target_link_libraries(${PROJECT_NAME}-finder hera-common
                      ${PROJECT_NAME}-broadcast)

if(WITH_SLAM)
  message(STATUS "Linking with libhera-slam")
  target_link_libraries(${PROJECT_NAME} hera-slam-result)
endif()

install(
  FILES script/hera-daemon.service
  PERMISSIONS OWNER_EXECUTE OWNER_READ
  DESTINATION /lib/systemd/system)
install(
  TARGETS hera-daemon
  PERMISSIONS OWNER_EXECUTE OWNER_READ
  DESTINATION bin)
install(
  TARGETS hera-daemon-finder
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION bin)
install(
  CODE "execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/script/install-extra.sh)"
)
