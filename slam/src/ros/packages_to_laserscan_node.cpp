#include <ros/ros.h>

#include "velodyne_accumulator.hpp"
#include "pointcloud2laserscan_velodyne.hpp"

int main(int argc, char** argv)
{
    ros::init(argc, argv, "packages_to_laserscan");

    ROS_INFO("NODE ENTRY");

    ros::NodeHandle nh_;
    ros::NodeHandle private_nh_("~");

    std::string laser_type = private_nh_.param<std::string>("laser_type", "velodyne");

    ROS_INFO("%s", laser_type.c_str());

    //instantiate an accumulator for translate "packages" to "pointcloud"
    if (laser_type.compare("velodyne") == 0) {
        std::unique_ptr<VelodyneAccumulator> accumulator(new VelodyneAccumulator(nh_, private_nh_));

        //TODO:Hard code subscribe topic name
        ros::Subscriber sub = nh_.subscribe("velodyne_points",
                10,
                &VelodyneAccumulator::original_points_callback,
                accumulator.get());

        if (sub) {
            ROS_INFO("Subscriber is valid");
        }
    } else {
        ROS_ERROR("laser type is not supported");
        return -1;
    }

    while(ros::ok())
    {
        ros::spin();
    }

    return 0;
}
