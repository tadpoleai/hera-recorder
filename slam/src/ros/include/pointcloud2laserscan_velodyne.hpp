
#pragma once

#include "message_filters/subscriber.h"
#include "sensor_msgs/PointCloud2.h"

class Pointcloud2LaserScanVelodyne  //:Pointcloud2LaserscanInterface
{
public:
    Pointcloud2LaserScanVelodyne(ros::NodeHandle& node, ros::NodeHandle& privateNode);

    ~Pointcloud2LaserScanVelodyne();

    void pointcloud_callback(const sensor_msgs::PointCloud2ConstPtr& cloud_msg);

protected:
    // ros handle
    ros::NodeHandle nh_;
    ros::NodeHandle private_nh_;

    // publisher
    ros::Publisher laserscan_pub_;

    std::string target_frame_;

    double min_height_;

    double max_height_;

    double horizontal_angle_min_;

    double horizontal_angle_max_;

    double vertical_angle_min_;

    double vertical_angle_max_;

    double range_min_;

    double range_max_;

    double horizontal_angle_increment_;

    std::string laserscan_output_topic_;
};
