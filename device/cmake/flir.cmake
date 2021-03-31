# Find FlyCapture
set(FLY_CAPTURE_INCLUDE_DIR /usr/include)
set(FLY_CAPTURE_LIBS /usr/lib/libflycapture.so)

if(ARCHITECTURE STREQUAL "arm64")
  message_plugin("Architecture is arm64")
  message_plugin("FlyCapture SDK not supported on arm64, exit")
  set(PLUGIN_EXIT 1)
endif()

if(NOT PLUGIN_EXIT)
  find_package(OpenCV 3.4 REQUIRED)
  if(OpenCV_FOUND)
    message_plugin("OpenCV found, version = ${OpenCV_VERSION}")

    if(EXISTS ${FLY_CAPTURE_LIBS}
       AND EXISTS ${FLY_CAPTURE_INCLUDE_DIR}/flycapture/FlyCapture2.h)
      message_plugin("FlyCapture found")
      set(PLUGIN_DRIVER_FOUND 1)

      set(PLUGIN_DRIVER_DEP_INCLUDE ${FLY_CAPTURE_INCLUDE_DIR}
                                    ${OpenCV_INCLUDE_DIRS})
      set(PLUGIN_DRIVER_DEP_LIBS ${FLY_CAPTURE_LIBS} ${OpenCV_LIBS})
    else()
      message_plugin("FlyCapture not found")
      set(PLUGIN_DRIVER_FOUND 0)
    endif()
  else()
    message_plugin("OpenCV not found")
    set(PLUGIN_DRIVER_FOUND 0)
  endif()
endif()
