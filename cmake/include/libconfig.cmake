find_library(LIBCONFIG config++)
if(NOT LIBCONFIG)
  message(FATAL_ERROR "${Escape}[31m" "LibConfig not found! CMake aborted!"
                      "${Escape}[m")
else()
  message(STATUS "${Escape}[32m" "LibConfig found: " ${LIBCONFIG} "${Escape}[m")
endif()
