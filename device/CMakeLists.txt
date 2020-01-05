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

# Build device (core)
include_directories(${PROJECT_NAME} "../" ${JPEG_TURBO_INCLUDE_DIR})
add_library(${PROJECT_NAME} STATIC ${ALL_SRC})
target_link_libraries(${PROJECT_NAME} hera-common hera-storage Threads::Threads
                      ${JPEG_TURBO_LIBS})

# Build device (drivers)
add_library(${PROJECT_NAME}-driver STATIC ${ALL_SRC})
target_include_directories(${PROJECT_NAME}-driver
                           PRIVATE ${FLY_CAPTURE_INCLUDE_DIR})
target_compile_definitions(${PROJECT_NAME}-driver PRIVATE WITH_DRIVER=1)
target_link_libraries(${PROJECT_NAME}-driver hera-common hera-storage
                      Threads::Threads ${JPEG_TURBO_LIBS} ${FLT_CAPTURE_LIBS})
