#pragma once

// #include "pointcloud_accumulator_api.hpp"

#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <pcl_ros/point_cloud.h>
#include <sensor_msgs/PointCloud2.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseStamped.h>
#include <pcl_ros/point_cloud.h>
#include <nav_msgs/Odometry.h>
#include <boost/circular_buffer.hpp>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/common/transforms.h>
#include <tf_conversions/tf_eigen.h>
#include <tf/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>

#include <eigen_conversions/eigen_msg.h>
#include <thread>
#include <mutex>
#include <condition_variable>

using PointT = pcl::PointXYZI;
using PointCloudPtr = pcl::PointCloud<PointT>::Ptr;
using Laser_Packet_Buffer = boost::circular_buffer<sensor_msgs::PointCloud2::ConstPtr>;

class VelodyneAccumulator //: public PointcloudAccumulatorInterface
{
public:
    VelodyneAccumulator(ros::NodeHandle& node, ros::NodeHandle& privateNode);

    ~VelodyneAccumulator();

    void original_points_callback(const sensor_msgs::PointCloud2::ConstPtr& points_msg);

    void accumulated_points_thread();

protected:
    // ros handle
    ros::NodeHandle node_;
    ros::NodeHandle mt_nh;
    ros::NodeHandle private_node_;

    // ros topic handler
    ros::Subscriber original_points_sub;
    ros::Publisher corrected_points_pub;

    // circular buffer to store laser packets
    Laser_Packet_Buffer packet_buffer_;

    // thread
    std::thread cloud_correct_thread_;

    // metux
    std::mutex cloud_correct_mutex_;
    std::condition_variable fill_buffer_cv_;

    // num packet to accumulate
    size_t num_packet_accumulate_;
};
