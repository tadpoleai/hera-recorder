# Find thrift library and IDL compiler
string(ASCII 27 Escape)
find_library(THRIFT thrift)
if(NOT THRIFT)
  # Try to install thrift
  message(
    STATUS "${Escape}[33m"
           "Thrift not found, invoking deploy/install-thrift.sh to install"
           "${Escape}[m")
  execute_process(
    COMMAND "sudo" "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/install-thrift.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/")
  # Check again
  find_library(THRIFT thrift)
  if(NOT THRIFT)
    message(
      FATAL_ERROR "${Escape}[31m" "Thrift installation failed! CMake aborted!"
                  "${Escape}[m")
  else()
    message(STATUS "${Escape}[32m" "Thrift installation to: " ${THRIFT}
                   " succeed" "${Escape}[m")
  endif()
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
