# Find FlyCapture
string(ASCII 27 Escape)
set(FLY_CAPTURE_INCLUDE_DIR /usr/include)
set(FLT_CAPTURE_LIBS /usr/lib/libflycapture.so)
if(EXISTS ${FLT_CAPTURE_LIBS}
   AND EXISTS ${FLY_CAPTURE_INCLUDE_DIR}/flycapture/FlyCapture2.h)
  message(STATUS "${Escape}[32m" "FlyCapture found" "${Escape}[m")
else()
  # Try to install FlyCapture
  message(
    STATUS
      "${Escape}[33m"
      "FlyCapture not found, invoking deploy/install-flycapture.sh to install"
      "${Escape}[m")
  execute_process(
    COMMAND "sudo" "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/install-flycapture.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/")
  # Check again
  if(EXISTS ${FLT_CAPTURE_LIBS}
     AND EXISTS ${FLY_CAPTURE_INCLUDE_DIR}/flycapture/FlyCapture2.h)
    message(STATUS "${Escape}[32m" "FlyCapture installation succeed"
                   "${Escape}[m")
  else()
    message(
      FATAL_ERROR
        "${Escape}[31m" "FlyCapture installation failed! CMake aborted!"
        "${Escape}[m")
  endif()
endif()
