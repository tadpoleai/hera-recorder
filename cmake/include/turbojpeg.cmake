# Find TurboJpeg
string(ASCII 27 Escape)

set(JPEG_TURBO_ROOT_DIR /opt/libjpeg-turbo)
set(JPEG_TURBO_INCLUDE_DIR ${JPEG_TURBO_ROOT_DIR}/include)
set(JPEG_TURBO_LIBRARY_DIR ${JPEG_TURBO_ROOT_DIR}/lib64)
set(JPEG_TURBO_LIBS ${JPEG_TURBO_LIBRARY_DIR}/libturbojpeg${DYNAMIC_LIB_EXT})

if(ARCHITECTURE STREQUAL "arm64")
  message(STATUS "Set Turbojpeg-arm")
  set(JPEG_TURBO_ROOT_DIR /opt/s32v/libjpeg-turbo)
  set(JPEG_TURBO_INCLUDE_DIR ${JPEG_TURBO_ROOT_DIR}/include)
  set(JPEG_TURBO_LIBRARY_DIR ${JPEG_TURBO_ROOT_DIR}/lib)
  set(JPEG_TURBO_LIBS ${JPEG_TURBO_LIBRARY_DIR}/libturbojpeg${DYNAMIC_LIB_EXT})
endif()

if(EXISTS ${JPEG_TURBO_LIBS} AND EXISTS ${JPEG_TURBO_INCLUDE_DIR}/turbojpeg.h)
  message(STATUS "${Escape}[32m" "TurboJpeg found" "${Escape}[m")
else()
  message(FATAL_ERROR "${Escape}[31m" "TurboJpeg not found! CMake aborted!"
                      "${Escape}[m")
endif()
