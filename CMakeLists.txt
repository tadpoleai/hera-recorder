cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera)
project(${PROJECT_NAME})

add_definitions(-DHERA_COMPILE_IN_REPO)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

include("cmake/include/system_arch.cmake")

option(WITH_GIT_INFO "Build with version information" ON)
option(WITH_ALL
       "Build hera with all components ( convert, daemon, replay, slam )" OFF)
option(WITH_CONVERT
       "Build with hera-convert( converting record to ROS Bag format )" OFF)
option(WITH_REPLAY "Build with hera-replay( replaying record via ipc )" OFF)
option(WITH_DAEMON "Build with hera-daemon( daemon for getting data )" OFF)
option(WITH_SLAM "Build with hera-slam( realtime slam )" OFF)

if(WITH_GIT_INFO)
  message(STATUS "Hera will build with version information")
  include(cmake/include/git_info.cmake)
else()
  message(STATUS "Hera will build without version information")
endif(WITH_GIT_INFO)

message(STATUS "Hera adding common")
add_subdirectory(common)

message(STATUS "Hera adding device")
add_subdirectory(device)

message(STATUS "Hera adding storage")
add_subdirectory(storage)

if(WITH_ALL)
  set(WITH_CONVERT ON)
  set(WITH_REPLAY ON)
  set(WITH_DAEMON ON)
  set(WITH_SLAM ON)
endif(WITH_ALL)

if(WITH_CONVERT)
  message(STATUS "Hera will build with convert")
  add_subdirectory(convert)
else()
  message(STATUS "Hera will build without convert")
endif(WITH_CONVERT)

if(WITH_REPLAY)
  message(STATUS "Hera will build with replay")
  add_subdirectory(replay)
else()
  message(STATUS "Hera will build without replay")
endif(WITH_REPLAY)

if(WITH_SLAM)
  add_definitions(-DWITH_SLAM)
  message(STATUS "Hera will build with slam")
  add_subdirectory(slam)
else()
  message(STATUS "Hera will build without slam")
endif(WITH_SLAM)

if(WITH_DAEMON)
  message(STATUS "Hera will build with daemon")
  add_subdirectory(daemon)
else()
  message(STATUS "Hera will build without daemon")
endif()
