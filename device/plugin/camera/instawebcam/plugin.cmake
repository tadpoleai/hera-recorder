# V4L2 webcam-based Insta fallback plugin

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(PLUGIN_DRIVER_FOUND 1)
  message_plugin("webcam driver enabled (V4L2)")
else()
  set(PLUGIN_DRIVER_FOUND 0)
  message_plugin("webcam driver disabled: only Linux/V4L2 is supported")
endif()
