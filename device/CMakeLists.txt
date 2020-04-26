cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-device)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")
include("../deploy/cmake/turbojpeg.cmake")

find_package(Threads REQUIRED)

file(GLOB_RECURSE DRIVERS_SRC "driver/*.cpp")
file(GLOB_RECURSE ALL_SRC "src/*.cpp")

# Drivers - Camera/Flir
if(DEVICE_DRIVER_FLIR)
  include("../deploy/cmake/flycapture.cmake")
endif()

# Drivers - Odometry/S32V Camera/S32V @note see s32vsal.cmake

# Build device (core)
include_directories(. include .. ${CMAKE_BINARY_DIR})
add_library(${PROJECT_NAME} SHARED src/device_factory.cpp ${ALL_SRC})
target_link_libraries(${PROJECT_NAME} hera-common hera-storage Threads::Threads
                      ${JPEG_TURBO_LIBS})
if(GIT_INFO_ENABLED)
  target_link_libraries(${PROJECT_NAME} hera-git-info)
endif()

# Build device (drivers)
add_library(${PROJECT_NAME}-driver SHARED src/device_factory.cpp ${DRIVERS_SRC}
                                          ${ALL_SRC})
target_include_directories(
  ${PROJECT_NAME}-driver PRIVATE ${FLY_CAPTURE_INCLUDE_DIR}
                                 ${JPEG_TURBO_INCLUDE_DIR})
target_compile_definitions(${PROJECT_NAME}-driver PRIVATE WITH_DRIVER=1)
target_link_libraries(
  ${PROJECT_NAME}-driver
  hera-common
  hera-storage
  Threads::Threads
  ${JPEG_TURBO_LIBS}
  ${FLY_CAPTURE_LIBS}
  ${S32VSAL_LIBS}
  ${S32VSAL_VSDK_LIBS})
if(GIT_INFO_ENABLED)
  target_link_libraries(${PROJECT_NAME}-driver hera-git-info)
endif()

# Install shared libraries
install(
  TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-driver
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION lib)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include/hera/device/)

# Build test
add_executable(${PROJECT_NAME}-test unit_test/dummy_test.cpp)
target_link_libraries(${PROJECT_NAME}-test hera-common hera-storage
                      ${PROJECT_NAME}-driver)

# Build record-test
add_executable(${PROJECT_NAME}-record unit_test/record_test.cpp)
target_link_libraries(${PROJECT_NAME}-record hera-common hera-storage
                      ${PROJECT_NAME}-driver)
