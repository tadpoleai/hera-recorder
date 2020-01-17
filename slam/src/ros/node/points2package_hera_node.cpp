#include <iostream>

#include <pcl/common/transforms.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>
#include <ros/ros.h>
#include <sensor_msgs/Imu.h>

#include "common/ipc/ipc_queue.hpp"
#include "common/logger/logger.hpp"
#include "device/include.hpp"
#include "message_filters/subscriber.h"
#include "sensor_msgs/PointCloud2.h"

using namespace wayz::hera;

using PointT = pcl::PointXYZI;
using PointCloudPtr = pcl::PointCloud<PointT>::Ptr;

static ros::Time to_ros_time(time::Timestamp ts)
{
    return ros::Time(ts.tv_sec, ts.tv_nsec);
}

int main(int argc, char** argv)
{
    log::onlyprint();

    ros::init(argc, argv, "points2package_hera");
    auto logger = log::info << "points2package_hera: ";

    ros::NodeHandle nh_;
    ros::NodeHandle private_nh_("~");

    auto ipc_queue = ipc::IPCQueue<device::data::SensorData>::create();
    ipc_queue->open(0, ipc::OpenMode::Read);

    std::string points_topic_ = private_nh_.param<std::string>("package_topic", "/velodyne_points");
    logger << "points_topic = " << points_topic_ << log::endl;

    std::string target_frame = private_nh_.param<std::string>("points_topic_", "velodyne");
    logger << "target_frame = " << target_frame << log::endl;

    ros::Publisher points_pub_ = nh_.advertise<sensor_msgs::PointCloud2>(points_topic_, 10, false);
    ros::Publisher imu_pub_ = nh_.advertise<sensor_msgs::Imu>("imu/raw_data", 10, false);

    ros::Rate sleep_rate(1e4);
    // TODO:read from recorder later
    constexpr uint32_t HorizontalLidarId = 1;
    while (ros::ok()) {
        auto data = ipc_queue->read();
        if (data && data->sensor_data_type == device::SensorDataType::PointsXYZI &&
            data->sensor_id == HorizontalLidarId) {
            auto data_impl = reinterpret_cast<device::data::PointsXYZI*>(data.get());

            // copy points
            pcl::PointCloud<PointT>::Ptr points(new pcl::PointCloud<PointT>());
            points->resize(data_impl->point_number);
            float* dst_ptr = reinterpret_cast<float*>(points->points.data());
            float* src_ptr = reinterpret_cast<float*>(data_impl->points);
            const float* dst_ptr_end = dst_ptr + (size_t)(data_impl->point_number) * (sizeof(PointT) / sizeof(float));

            using Pt = device::data::PointsXYZI::PointXYZI;
            for (; dst_ptr < dst_ptr_end;) {
                memcpy(dst_ptr, src_ptr, sizeof(Pt));
                dst_ptr += sizeof(PointT) / sizeof(float);
                src_ptr += sizeof(Pt) / sizeof(float);
            }

            // construct sensor msg
            sensor_msgs::PointCloud2::Ptr q_msg(new sensor_msgs::PointCloud2());
            pcl::toROSMsg(*points, *q_msg);
            q_msg->header.seq = data_impl->sequence;
            q_msg->header.stamp = to_ros_time(data_impl->timestamp_intrinsic_ns);
            q_msg->header.frame_id = target_frame;

            // publish ros msg
            points_pub_.publish(q_msg);
        } else if (data && device::SensorDataType::ImuMagneticField == data->sensor_data_type) {
            auto data_impl = reinterpret_cast<device::data::ImuMagneticField*>(data.get());
            sensor_msgs::Imu imu_message;
            imu_message.header.frame_id = "imu_link";
            imu_message.header.stamp = to_ros_time(data_impl->timestamp_intrinsic_ns);
            imu_message.linear_acceleration.x = data_impl->linear_acceleration[0];
            imu_message.linear_acceleration.y = data_impl->linear_acceleration[1];
            imu_message.linear_acceleration.z = data_impl->linear_acceleration[2];
            imu_message.angular_velocity.x = data_impl->angular_velocity[0];
            imu_message.angular_velocity.y = data_impl->angular_velocity[1];
            imu_message.angular_velocity.z = data_impl->angular_velocity[2];

            imu_pub_.publish(imu_message);
        }

        else {
            ros::spinOnce();
            sleep_rate.sleep();
        }
    }
}
