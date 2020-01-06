# Check PCL Version
string(ASCII 27 Escape)
execute_process(
  COMMAND "pkg-config" "--list-all"
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE LIST_ALL)
string(REGEX MATCH "pcl_common-[^ \t\n]*" PCL_COMMON_VERSION ${LIST_ALL})

if(NOT PCL_COMMON_VERSION)
  message(STATUS "${Escape}[33m"
                 "PCL not found, invoking deploy/install-pcl.sh to install"
                 "${Escape}[m")
  execute_process(
    COMMAND "sudo" "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/install-pcl.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/")
  execute_process(
    COMMAND "pkg-config" "--list-all"
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE LIST_ALL)
  string(REGEX MATCH "pcl_common-[^ \t\n]*" PCL_COMMON_VERSION ${LIST_ALL})
  if(NOT PCL_COMMON_VERSION)
    message(FATAL_ERROR "${Escape}[31m"
                        "PCL installation failed! CMake aborted!" "${Escape}[m")
  else()
    message(STATUS "${Escape}[32m"
                   "PCL ${PCL_COMMON_VERSION} installation succeed"
                   "${Escape}[m")
  endif()
else()
  message(STATUS "${Escape}[32m" "PCL ${PCL_COMMON_VERSION} found!"
                 "${Escape}[m")
endif()

string(REGEX MATCH "pcl_common-(.*)" PCL_VERSION_NUMBER ${PCL_COMMON_VERSION})
set(PCL_INCLUDE_DIRS /usr/include/pcl-${CMAKE_MATCH_1})
