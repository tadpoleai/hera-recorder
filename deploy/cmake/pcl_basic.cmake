# Check PCL 1.7
string(ASCII 27 Escape)
set(PCL_INCLUDE_DIRS /usr/include/pcl-1.7)
if(NOT EXISTS "${PCL_INCLUDE_DIRS}/pcl/point_cloud.h")
  # Try to install pcl
  message(STATUS "${Escape}[33m"
                 "PCL 1.7 not found, invoking deploy/install-pcl.sh to install"
                 "${Escape}[m")
  execute_process(
    COMMAND "sudo" "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/install-pcl.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/")
  # Check again
  if(NOT EXISTS "${PCL_INCLUDE_DIRS}/pcl/point_cloud.h")
    message(
      FATAL_ERROR "${Escape}[31m" "PCL 1.7 installation failed! CMake aborted!"
                  "${Escape}[m")
  else()
    message(STATUS "${Escape}[32m" "PCL 1.7 installation succeed" "${Escape}[m")
  endif()
else()
  message(STATUS "${Escape}[32m" "PCL 1.7 found" "${Escape}[m")
endif()
