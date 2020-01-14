#pragma once

// #include "pointcloud_accumulator_api.hpp"

#include <condition_variable>
#include <mutex>
#include <thread>

#include <boost/circular_buffer.hpp>
#include <eigen_conversions/eigen_msg.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseStamped.h>
#include <nav_msgs/Odometry.h>
#include <pcl/common/transforms.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>
#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <tf/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>
#include <tf_conversions/tf_eigen.h>

using PointT = pcl::PointXYZI;
using PointCloudPtr = pcl::PointCloud<PointT>::Ptr;
using Laser_Packet_Buffer = boost::circular_buffer<sensor_msgs::PointCloud2::ConstPtr>;

class AccumulatorVelodyne  //: public PointcloudAccumulatorInterface
{
public:
    AccumulatorVelodyne(ros::NodeHandle& node, ros::NodeHandle& privateNode);

    ~AccumulatorVelodyne();

    void original_points_callback(const sensor_msgs::PointCloud2::ConstPtr& points_msg);

    void accumulated_points_thread();

protected:
    // ros handle
    ros::NodeHandle nh_;
    // ros::NodeHandle mt_nh;
    ros::NodeHandle private_nh_;

    // publisher
    ros::Publisher accumulated_points_pub_;

    // circular buffer to store laser packets
    Laser_Packet_Buffer packet_buffer_;

    // thread
    std::thread cloud_accumulate_thread_;

    // metux
    std::mutex cloud_accumulate_mutex_;
    std::condition_variable fill_buffer_cv_;

    // num packet to accumulate
    uint32_t num_packet_accumulate_;

    // accumulated points topic
    std::string accumulated_topic_;

    // trarget frame
    std::string target_frame_;
};
