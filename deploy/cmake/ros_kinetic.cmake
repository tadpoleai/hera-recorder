# Check ROS Kinetic
string(ASCII 27 Escape)
set(ROS_DISTRO "kinetic")
execute_process(
  COMMAND "rosversion" "-ds"
  OUTPUT_VARIABLE ROS_INSTALLED_DISTRO
  RESULT_VARIABLE ROS_CHECK)
if(${ROS_CHECK} OR NOT ${ROS_DISTRO} STREQUAL ${ROS_INSTALLED_DISTRO})
  message(
    FATAL_ERROR
      "${Escape}[31m"
      "ROS ${ROS_DISTRO} is not properly installed! CMake aborted!"
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
      "ROS ${ROS_DISTRO} installed but setup.bash is not sourced! CMake aborted!"
      "${Escape}[m")
endif()
get_filename_component(ROS_ROOT_DIR "${ROS_BAG_PATH}/../.." ABSOLUTE DIRECTORY)
