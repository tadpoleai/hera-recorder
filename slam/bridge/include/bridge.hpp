///
/// @file ros_bridge.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of Node to bridge between ROS and Hera
/// @version 0.2
/// @date 2020-02-04
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <ros/ros.h>
// Messages
#include <nav_msgs/OccupancyGrid.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/LaserScan.h>
#include <visualization_msgs/MarkerArray.h>

#include "common/ipc/ipc_queue.hpp"
#include "device/include.hpp"

namespace wayz {
namespace hera {
namespace slam {

///
/// @brief Bridge data / messages between ROS and Hera
///
class Bridge {
public:
    ///
    /// @brief Construct a new Bridge object
    ///
    /// @param nh ROS node handler
    /// @param private_nh ROS private node handler
    ///
    Bridge(ros::NodeHandle& nh, ros::NodeHandle& private_nh);

    ///
    /// @brief Destroy the Bridge objectÎ
    ///
    ~Bridge();

    ///
    /// @brief Block and wait for message
    ///
    void spin();

private:
    ///
    /// @brief 2D Vector of position
    ///
    struct Vector2d {
        float x;
        float y;
    };

private:
    ///
    /// @brief Handler for lidar data from hera
    ///
    /// @param data lidar data
    ///
    void lidar_handler(const device::data::PointsXYZI* const data);

    ///
    /// @brief Handler for imu data from hera
    ///
    /// @param data imu data
    ///
    void imu_handler(const device::data::ImuMagneticField* const data);

    ///
    /// @brief Callbakc handler for occupancy grid map message from ROS
    ///
    /// @param msg ros messsage
    ///
    void map_handler(const nav_msgs::OccupancyGrid::ConstPtr& msg);

    ///
    /// @brief Callback handler for markder array message from ROS
    ///
    /// @param msg ros message
    ///
    void trajectory_handler(const visualization_msgs::MarkerArray::ConstPtr& msg);

    /// Convert time::Timestamp to ROS Time
    inline ros::Time to_ros_time(const time::Timestamp& ts)
    {
        return ros::Time(ts.tv_sec, ts.tv_nsec);
    }

private:
    ros::NodeHandle nh_;              ///< ROS node handler
    ros::NodeHandle private_nh_;      ///< ROS private node handler
    ros::Publisher scan_pub_;         ///< ROS Publisher for sensor_msgs::LaserScan
    ros::Publisher imu_pub_;          ///< ROS Publisher for sensor_msgs::Imu
    ros::Subscriber map_sub_;         ///< ROS Subscriber for occupancy grid map
    ros::Subscriber trajectory_sub_;  ///< ROS Subscriber for trajectroy (marker array)

    static constexpr auto SleepRate = 2e3;  ///< rate [hz]
    ros::Rate rate_;                        ///< rate for waiting data from hera

    std::unique_ptr<ipc::IPCQueue<device::data::SensorData>> ipc_queue_;  ///< ipc queue interface from hera

private:
    /// Hera goes ROS Parameters
    int lidar_sensor_id_;  ///< Restrict sensor id for lidar, only accept id-matched data, -1 to disable
    int imu_sensor_id_;    ///< Restrict sensor id for lidar, only accept id-matched data, -1 to disable

    std::string scan_topic_;        ///< ROS Topic name for publishing sensor_msgs::LaserScan message
    std::string imu_topic_;         ///< ROS Topic name for publishing sensor_msgs::Imu message
    std::string scan_frame_;        ///< ROS Frame Id of published sensor_msgs::LaserScan message
    std::string imu_frame_;         ///< ROS Frame Id of published sensor_msgs::Imu message
    std::string map_topic_;         ///< ROS Topic name of occupancy grid map
    std::string trajectory_topic_;  ///< ROS Topic name of trajectroy (marker array)

    double min_height_;  ///< Minimum height[m] for 3d points to LaserScan
    double max_height_;  ///< Maximum height[m] for 3d points to LaserScan
    double min_pitch_;   ///< Minimum pitch[rad] for 3d points to LaserScan
    double max_pitch_;   ///< Maximum pitch[rad] for 3d points to LaserScan
    double min_range_;   ///< Maximum range[m] for 3d points to LaserScan
    double max_range_;   ///< Maximum range[m] for 3d points to LaserScan

    int azimuth_grids_;  ///< Num of grids on a whole circle for published LaserScan

private:
    bool scan_msg_inited_;              ///< Flag indicating if scan_msg is a whole circle
    float last_azimuth_;                ///< Last azimuth of input 3d points
    time::Timestamp last_circle_time_;  ///< Last timestamp of azimuth overcross 0
    sensor_msgs::LaserScan scan_msg_;   ///< LaserScan message to publish

    static constexpr size_t TrajectorySizeStep = 86400;  ///< Size of trajectory to reserver
    std::vector<Vector2d> trajectory_;                   ///< Trajectory of robot;
};

}  // namespace slam
}  // namespace hera
}  // namespace wayz