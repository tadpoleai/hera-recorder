# Find thrift library and IDL compiler
string(ASCII 27 Escape)

if(ARCHITECTURE STREQUAL "arm64")
  message(STATUS "Set Thrift-ARM")
  set(THRIFT_ROOT_DIR /opt/s32v/thrift)
  set(THRIFT_INCLUDE_DIR ${THRIFT_ROOT_DIR}/include)
  set(THRIFT_LIBRARY_DIR ${THRIFT_ROOT_DIR}/lib)
  set(THRIFT_LIBS ${THRIFT_LIBRARY_DIR}/libthrift.so)
  set(THRIFT ${THRIFT_LIBS})
  if(EXISTS ${THRIFT_LIBS} AND EXISTS ${THRIFT_INCLUDE_DIR}/thrift/Thrift.h)
    message(STATUS "${Escape}[32m" "Thrift-ARM found" "${Escape}[m")
  else()
    message(FATAL_ERROR "${Escape}[31m" "Thrift-ARM not found! CMake aborted!"
                        "${Escape}[m")
  endif()
else() # AMD64
  find_library(THRIFT thrift)
  if(NOT THRIFT)
    message(FATAL_ERROR "${Escape}[31m" "Thrift not found! CMake aborted!"
                        "${Escape}[m")
  else()
    message(STATUS "${Escape}[32m" "Thrift found: " ${THRIFT} "${Escape}[m")
  endif()

  # Check thrift IDL compiler
  execute_process(
    COMMAND "thrift" "--version"
    OUTPUT_QUIET
    RESULT_VARIABLE THRIFT_CHECK)
  if(${THRIFT_CHECK})
    message(
      FATAL_ERROR "${Escape}[31m" "Thrift version check failed! CMake aborted!"
                  "${Escape}[m")
  endif()
endif() # AMD64
