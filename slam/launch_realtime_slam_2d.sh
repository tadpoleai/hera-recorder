#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e

source $(dirname ${BASH_SOURCE[0]})/setup.bash

roslaunch carto_ros my_robot_2d.launch & \
./$(dirname ${BASH_SOURCE[0]})/points2package_hera_node & \
./$(dirname ${BASH_SOURCE[0]})/packages_to_laserscan_node