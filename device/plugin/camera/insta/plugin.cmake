# Insta360 Camera SDK discovery
# Priority:
# 1) -DINSTA_SDK_ROOT=/path
# 2) $ENV{INSTA_SDK_ROOT}
# 3) Common install locations (auto scan)

set(PLUGIN_DRIVER_FOUND 0)

set(_INSTA_CANDIDATE_ROOTS)

if(DEFINED INSTA_SDK_ROOT)
  list(APPEND _INSTA_CANDIDATE_ROOTS ${INSTA_SDK_ROOT})
elseif(DEFINED ENV{INSTA_SDK_ROOT})
  list(APPEND _INSTA_CANDIDATE_ROOTS $ENV{INSTA_SDK_ROOT})
else()
  list(APPEND _INSTA_CANDIDATE_ROOTS
       /usr/local
       /usr
       /opt/InstaSDK
       /opt/insta_sdk
       /opt/insta360-sdk
       /opt/camera_sdk
       /home/$ENV{USER}/InstaSDK
       /home/$ENV{USER}/insta_sdk)
endif()

find_path(_INSTA_INCLUDE_DIR
          NAMES camera/camera.h
          PATHS ${_INSTA_CANDIDATE_ROOTS}
          PATH_SUFFIXES include .
          NO_DEFAULT_PATH)

find_library(_INSTA_SDK_LIB
             NAMES CameraSDK camera_sdk insta_camera_sdk ins_camera_sdk
             PATHS ${_INSTA_CANDIDATE_ROOTS}
             PATH_SUFFIXES lib lib64 .
             NO_DEFAULT_PATH)

if(_INSTA_INCLUDE_DIR AND _INSTA_SDK_LIB)
  get_filename_component(_INSTA_ROOT ${_INSTA_INCLUDE_DIR} DIRECTORY)
  get_filename_component(_INSTA_ROOT ${_INSTA_ROOT} DIRECTORY)
  message_plugin("Insta SDK found at ${_INSTA_ROOT}")
  set(PLUGIN_DRIVER_FOUND 1)
  set(PLUGIN_DRIVER_DEP_INCLUDE ${_INSTA_INCLUDE_DIR})
  set(PLUGIN_DRIVER_DEP_LIBS ${_INSTA_SDK_LIB})

  find_library(_INSTA_UDEV_LIB NAMES udev)
  if(_INSTA_UDEV_LIB)
    message_plugin("libudev found at ${_INSTA_UDEV_LIB}")
    list(APPEND PLUGIN_DRIVER_DEP_LIBS ${_INSTA_UDEV_LIB})
  else()
    message_plugin("libudev not found; continue without explicit udev link")
  endif()
else()
  message_plugin("Insta SDK not found, set INSTA_SDK_ROOT to SDK path containing include/camera/camera.h and lib/libCameraSDK.so")
  set(PLUGIN_DRIVER_FOUND 0)
endif()
