cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera)
project(${PROJECT_NAME})

add_definitions(-DHERA_COMPILE_IN_REPO)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

include("cmake/include/system_arch.cmake")

if(NOT without-git-info)
  message(STATUS "Hera will build with git info")
  include(cmake/include/git_info.cmake)
else()
  message(STATUS "Hera will build without git info")
endif()

message(STATUS "Hera adding common")
add_subdirectory(common)

message(STATUS "Hera adding device")
add_subdirectory(device)

message(STATUS "Hera adding storage")
add_subdirectory(storage)

if(with-convert
   OR with-all
   AND NOT without-convert)
  message(STATUS "Hera will build with convert")
  add_subdirectory(convert)
else()
  message(STATUS "Hera will build without convert")
endif()

if(with-replay
   OR with-all
   AND NOT without-replay)
  message(STATUS "Hera will build with replay")
  add_subdirectory(replay)
else()
  message(STATUS "Hera will build without replay")
endif()

if(with-slam
   OR with-all
   AND NOT without-slam)
  set(WITH_SLAM 1)
  add_definitions(-DWITH_SLAM)
  message(STATUS "Hera will build with slam")

  add_subdirectory(slam)
else()
  message(STATUS "Hera will build without slam")
endif()

if(with-daemon
   OR with-all
   AND NOT without-daemon)
  message(STATUS "Hera will build with daemon")
  add_subdirectory(daemon)
else()
  message(STATUS "Hera will build without daemon")
endif()
