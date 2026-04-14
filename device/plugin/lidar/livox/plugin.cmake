# Livox SDK2 discovery
# Priority:
# 1) -DLIVOX_SDK2_ROOT=/path
# 2) $ENV{LIVOX_SDK2_ROOT}
# 3) /usr/local

set(PLUGIN_DRIVER_FOUND 0)

if(DEFINED LIVOX_SDK2_ROOT)
  set(_LIVOX_ROOT ${LIVOX_SDK2_ROOT})
elseif(DEFINED ENV{LIVOX_SDK2_ROOT})
  set(_LIVOX_ROOT $ENV{LIVOX_SDK2_ROOT})
else()
  set(_LIVOX_ROOT /usr/local)
endif()

set(_LIVOX_INCLUDE_DIR ${_LIVOX_ROOT}/include)

find_library(_LIVOX_SDK_LIB
             NAMES livox_lidar_sdk_shared livox_lidar_sdk_static livox_lidar_sdk
             PATHS ${_LIVOX_ROOT}/lib ${_LIVOX_ROOT}/lib64
             NO_DEFAULT_PATH)

if(EXISTS ${_LIVOX_INCLUDE_DIR}/livox_lidar_api.h AND _LIVOX_SDK_LIB)
  message_plugin("Livox SDK2 found at ${_LIVOX_ROOT}")
  set(PLUGIN_DRIVER_FOUND 1)
  set(PLUGIN_DRIVER_DEP_INCLUDE ${_LIVOX_INCLUDE_DIR})
  set(PLUGIN_DRIVER_DEP_LIBS ${_LIVOX_SDK_LIB})
else()
  message_plugin("Livox SDK2 not found, expected include in ${_LIVOX_INCLUDE_DIR}")
  set(PLUGIN_DRIVER_FOUND 0)
endif()
