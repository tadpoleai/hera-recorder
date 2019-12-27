cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-device)
project(${PROJECT_NAME})

include("../deploy/cmake/cpp_common.cmake")
include("../deploy/cmake/turbojpeg.cmake")

if(DEVICE_DRIVER_FLIR)
  message(STATUS "DEVICE_DRIVER_FLIR")
  include("../deploy/cmake/flycapture.cmake")
endif()

find_package(Threads REQUIRED)

file(GLOB_RECURSE ALL_SRC "src/*.cpp" "src/**/*.cpp")

include_directories("../" ${FLY_CAPTURE_INCLUDE_DIR} ${JPEG_TURBO_INCLUDE_DIR})
add_library(${PROJECT_NAME} STATIC ${ALL_SRC})
target_link_libraries(${PROJECT_NAME} hera-common hera-storage Threads::Threads
                      ${FLT_CAPTURE_LIBS} ${JPEG_TURBO_LIBS})
