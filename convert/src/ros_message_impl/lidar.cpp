///
/// @file lidar.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class ROSMessage for lidar
/// @date 2019-12-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>

#include "../ros_message_impl.hpp"

namespace wayz {
namespace hera {
namespace convert {

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::PointsXYZI>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::PointsXYZI*>(sensor_data.get());
    auto message = ROSMessage::create<ROSMessageType::PointCloud2>();
    auto ros_message = reinterpret_cast<sensor_msgs::PointCloud2*>(message->ptr);
    std::vector<ROSMessagePtr> ret;

    message->topic_name = remapper->remap(topic_prefix + "point_cloud2");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    pcl::PointCloud<pcl::PointXYZI> pcl_cloud;
    pcl_cloud.resize(data_impl->point_number);
    float* dst_ptr = reinterpret_cast<float*>(pcl_cloud.points.data());
    float* src_ptr = reinterpret_cast<float*>(data_impl->points);
    const float* dst_ptr_end = dst_ptr + (size_t)(data_impl->point_number) * (sizeof(pcl::PointXYZI) / sizeof(float));

    using Pt = device::data::PointsXYZI::PointXYZI;
    for (; dst_ptr < dst_ptr_end;) {
        memcpy(dst_ptr, src_ptr, sizeof(Pt));
        dst_ptr += sizeof(pcl::PointXYZI) / sizeof(float);
        src_ptr += sizeof(Pt) / sizeof(float);
    }
    pcl::toROSMsg(pcl_cloud, *ros_message);
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    ret.emplace_back(std::move(message));
    return ret;
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
