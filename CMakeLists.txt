cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera)
project(${PROJECT_NAME})

if(target-arm)
  set(TARGET_ARM 1)
  add_definitions(-DTARGET_ARM)
  message(STATUS "Hera will build for arm")
endif()

if(NOT without-git-info)
  message(STATUS "Hera will build with git info")
  include(deploy/cmake/git_info.cmake)
else()
  message(STATUS "Hera will build without git info")
endif()

message(STATUS "Hera adding common")
add_subdirectory(common)

message(STATUS "Hera adding storage")
add_subdirectory(storage)

# Whether to build lib-hera-devices with some external-library-required device
# drivers, list them here to pass the definitions to all subdirectory. Devices
# built without driver can be be open by record-daemon, or it will return return
# HeraErrno::UnimplementedDriver;
if(with-driver-flir OR with-driver-all)
  set(DEVICE_DRIVER_FLIR 1)
  add_definitions(-DDEVICE_DRIVER_FLIR)
  message(STATUS "Hera will build with device driver flir")
else()
  message(STATUS "Hera will build without device driver flir")
endif()

if(with-driver-s32vmipi OR with-driver-arm)
  set(DEVICE_DRIVER_S32VMIPI 1)
  add_definitions(-DDEVICE_DRIVER_S32VMIPI)
  message(STATUS "Hera will build with device driver s32vmipi")
else()
  message(STATUS "Hera will build without device driver s32vmipi")
endif()

message(STATUS "Hera adding device")
add_subdirectory(device)

if(with-daemon
   OR with-all
   AND NOT without-daemon)
  message(STATUS "Hera will build with daemon")
  add_subdirectory(daemon)
else()
  message(STATUS "Hera will build without daemon")
endif()

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

if(with-client
   OR with-all
   AND NOT without-client)
  message(STATUS "Hera will build with client")
  add_subdirectory(client)
else()
  message(STATUS "Hera will build without client")
endif()
