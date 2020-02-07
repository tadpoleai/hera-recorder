#!/bin/bash

source /opt/ros/kinetic/setup.bash

export PYTHONPATH=/opt/ros/kinetic/lib/python2.7/dist-packages

roscore & 
sleep 3

rosparam set robot_description "'`cat /usr/local/share/hera/slam/carto/share/urdf/tron_4.urdf`'"
sleep 1

/opt/ros/kinetic/lib/robot_state_publisher/robot_state_publisher&
sleep 1

hera-slam-bridge &
sleep 1

carto_node \
-configuration_directory /usr/local/share/hera/slam/carto/share/config \
-configuration_basename my_robot_2d.lua \
-minloglevel 9 &

carto_occupancy_grid_node \
-resolution 0.1 \
-minloglevel 9 &
