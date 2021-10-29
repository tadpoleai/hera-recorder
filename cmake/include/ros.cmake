# Check ROS Kinetic
string(ASCII 27 Escape)
set(ROS_DISTRO_16 "kinetic")
set(ROS_DISTRO_18 "melodic")
set(ROS_DISTRO_20 "noetic")
execute_process(
  COMMAND "rosversion" "-d"
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE ROS_INSTALLED_DISTRO
  RESULT_VARIABLE ROS_CHECK)
if(${ROS_CHECK} OR (NOT ${ROS_DISTRO_16} STREQUAL ${ROS_INSTALLED_DISTRO}
                    AND NOT ${ROS_DISTRO_18} STREQUAL ${ROS_INSTALLED_DISTRO}
                    AND NOT ${ROS_DISTRO_20} STREQUAL ${ROS_INSTALLED_DISTRO}))
  message(
    FATAL_ERROR
      "${Escape}[31m"
      "ROS ${ROS_DISTRO_16} nor ROS ${ROS_DISTRO_18} nor ROS ${ROS_DISTRO_20} is not properly installed! CMake aborted!"
      "${Escape}[m")
endif()

# Locate ROS installed path
execute_process(
  COMMAND "which" "rosbag"
  OUTPUT_VARIABLE ROS_BAG_PATH
  RESULT_VARIABLE ROS_CHECK)
if(${ROS_CHECK})
  message(
    FATAL_ERROR
      "${Escape}[31m"
      "ROS ${ROS_INSTALLED_DISTRO} installed but setup.bash is not sourced! CMake aborted!"
      "${Escape}[m")
endif()

get_filename_component(ROS_ROOT_DIR "${ROS_BAG_PATH}/../.." ABSOLUTE DIRECTORY)

message(STATUS "${Escape}[32m" "ROS ${ROS_INSTALLED_DISTRO} Found"
               "${Escape}[m")
