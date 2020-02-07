///
/// @file bridge_node.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief ROS Node of slam-ros-bridge
/// @version 0.1
/// @date 2020-02-07
///
/// @copyright Copyright (c) 2020
///

#include <iostream>

#include "bridge.hpp"

using namespace wayz::hera;

int main(int argc, char** argv)
{
    log::onlyprint();

    ros::init(argc, argv, "hera_slam_ros_bridge");
    log::info << "Slam: ros-bridge inited" << log::endl;

    ros::NodeHandle nh;
    ros::NodeHandle private_nh("~");

    slam::Bridge bridge(nh, private_nh);
    bridge.spin();
}
