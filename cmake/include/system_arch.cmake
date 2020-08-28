# Target Arch
if(NOT DEFINED ENV{ARCH})
  execute_process(
    COMMAND uname -m
    COMMAND tr -d '\n'
    OUTPUT_VARIABLE ARCHITECTURE)
else()
  set(ARCHITECTURE $ENV{ARCH})
endif()

add_definitions(-DARCHITECTURE="${ARCHITECTURE}")
message(STATUS "Architecture is ${ARCHITECTURE}")

# Host Sysname
execute_process(
  COMMAND uname -s
  COMMAND tr -d '\n'
  OUTPUT_VARIABLE SYSTEMNAME)

if(SYSTEMNAME STREQUAL "Linux")
  set(DYNAMIC_LIB_EXT ".so")
  set(LINUX 1)
  add_definitions(-DLINUX)
elseif(SYSTEMNAME STREQUAL "Darwin")
  set(DYNAMIC_LIB_EXT ".dylib")
  set(DARWIN 1)
  add_definitions(-DDARWIN)
else()
  message(FATAL "Unexcepted System name ${SYSTEMNAME}")
endif()

add_definitions(-DSYSTEMNAME="${SYSTEMNAME}")
add_definitions(-DDYNAMIC_LIB_EXT="${DYNAMIC_LIB_EXT}")
message(STATUS "System name is ${SYSTEMNAME}")
