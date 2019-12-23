# Find TurboJpeg
string(ASCII 27 Escape)
set(JPEG_TURBO_ROOT_DIR /opt/libjpeg-turbo)
set(JPEG_TURBO_INCLUDE_DIR ${JPEG_TURBO_ROOT_DIR}/include)
set(JPEG_TURBO_LIBRARY_DIR ${JPEG_TURBO_ROOT_DIR}/lib64)
set(JPEG_TURBO_LIBS ${JPEG_TURBO_LIBRARY_DIR}/libturbojpeg.so)
if(EXISTS ${JPEG_TURBO_LIBS} AND EXISTS ${JPEG_TURBO_INCLUDE_DIR}/turbojpeg.h)
  message(STATUS "${Escape}[32m" "TurboJpeg found" "${Escape}[m")
else()
  # Try to install TurboJpeg
  message(
    STATUS
      "${Escape}[33m"
      "TurboJpeg not found, invoking deploy/install-jpeg-turbo.sh to install"
      "${Escape}[m")
  execute_process(
    COMMAND "sudo" "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/install-jpeg-turbo.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/")
  # Check again
  if(EXISTS ${JPEG_TURBO_LIBS} AND EXISTS ${JPEG_TURBO_INCLUDE_DIR}/turbojpeg.h)
    message(STATUS "${Escape}[32m" "TurboJpeg installation succeed"
                   "${Escape}[m")
  else()
    message(
      FATAL_ERROR "${Escape}[31m"
                  "TurboJpeg installation failed! CMake aborted!" "${Escape}[m")
  endif()
endif()
