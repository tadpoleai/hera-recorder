cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-device)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")
include("../deploy/cmake/turbojpeg.cmake")

find_package(Threads REQUIRED)

file(GLOB_RECURSE ALL_SRC "src/*.cpp")

# Drivers - Camera/Flir
if(DEVICE_DRIVER_FLIR)
  include("../deploy/cmake/flycapture.cmake")
endif()

# Drivers - Camera/S32VMipi
if(DEVICE_DRIVER_S32VMIPI)
  set(S32V_VSDK_LIBS ${CMAKE_CURRENT_SOURCE_DIR}/lib/arm/libvsdk_ar0143.so)
endif()

include_directories(${PROJECT_NAME} "../" include)

# Build device (core)
add_library(${PROJECT_NAME} SHARED ${ALL_SRC})
target_link_libraries(${PROJECT_NAME} hera-common hera-storage Threads::Threads
                      ${JPEG_TURBO_LIBS})

# Build device (drivers)
add_library(${PROJECT_NAME}-driver SHARED ${ALL_SRC})
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
  ${FLT_CAPTURE_LIBS}
  ${S32V_VSDK_LIBS})

# Install shared libraries
install(
  TARGETS ${PROJECT_NAME} ${PROJECT_NAME}-driver
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION lib)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include/hera/device/)

if(TARGET_ARM)
  # Build s32vmipi test
  add_executable(${PROJECT_NAME}-s32vmipi-test unit_test/s32vmipi_test.cpp)
  target_link_libraries(
    ${PROJECT_NAME}-s32vmipi-test
    hera-common
    hera-storage
    ${PROJECT_NAME}-driver
    Threads::Threads
    ${JPEG_TURBO_LIBS}
    ${FLT_CAPTURE_LIBS}
    ${S32V_VSDK_LIB})
endif()

# Build test
add_executable(${PROJECT_NAME}-test unit_test/dummy_test.cpp)
target_link_libraries(
  ${PROJECT_NAME}-test
  hera-common
  hera-storage
  ${PROJECT_NAME}-driver
  Threads::Threads
  ${JPEG_TURBO_LIBS}
  ${FLT_CAPTURE_LIBS})
