#include <ros/ros.h>

#include "message_filters/subscriber.h"
#include "package_accumulator_velodyne.hpp"
#include "pointcloud2laserscan_velodyne.hpp"
#include "sensor_msgs/PointCloud2.h"

int main(int argc, char** argv)
{
    ros::init(argc, argv, "packages_to_laserscan");

    ROS_INFO("NODE ENTRY");

    ros::NodeHandle nh_;
    ros::NodeHandle private_nh_("~");

    // parameter 01:"laser_type"
    std::string laser_type = private_nh_.param<std::string>("laser_type", "velodyne");

    // parameter 02:"original package topic"
    std::string package_topic = private_nh_.param<std::string>("package_topic", "/velodyne_points");

    ROS_INFO("%s", laser_type.c_str());
    ROS_INFO("%s", package_topic.c_str());

    // instantiate an accumulator for translate "packages" to "pointcloud"
    std::unique_ptr<AccumulatorVelodyne> accumulator(new AccumulatorVelodyne(nh_, private_nh_));
    std::unique_ptr<Pointcloud2LaserScanVelodyne> translator(new Pointcloud2LaserScanVelodyne(nh_, private_nh_));
    message_filters::Subscriber<sensor_msgs::PointCloud2> pointcloud_sub;
    std::string accumulated_topic = private_nh_.param<std::string>("accumulated_topic", "accumulated_points");
    pointcloud_sub.subscribe(nh_, accumulated_topic, 10);
    ros::Subscriber packages_sub;

    if (laser_type.compare(laser_type) == 0) {
        packages_sub =
                nh_.subscribe(package_topic, 10, &AccumulatorVelodyne::original_points_callback, accumulator.get());

        if (packages_sub) {
            ROS_INFO("Subscriber is valid");
        }

        // subsrcibe "pointcloud" published by AccumulatorVelodyne
        pointcloud_sub.registerCallback(
                boost::bind(&Pointcloud2LaserScanVelodyne::pointcloud_callback, translator.get(), _1));

    } else {
        ROS_ERROR("laser type is not supported");
        return -1;
    }

    ros::Rate sleep_rate(1e4);
    while (ros::ok()) {
        ros::spinOnce();
        sleep_rate.sleep();
    }

    return 0;
}
