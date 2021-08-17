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
#include <sensor_msgs/PointField.h>

#include "ros_message_impl.hpp"

namespace wayz {
namespace hera {
namespace convert {

#pragma pack(push, 1)
struct PointXYZIRT {
    float x;
    float y;
    float z;
    float intensity;
    uint16_t ring;
    float time;
};
#pragma pack(pop)

static std::vector<ROSMessagePtr> convertXYZIRTCloud(device::data::SensorDataPtr& sensor_data,
                                                     const std::string& topic_prefix,
                                                     const std::string& frame_id,
                                                     const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::Points*>(sensor_data.get());
    std::vector<ROSMessagePtr> ret;

    if (data_impl->meta.return_type == device::data::Points::ReturnType::Dual) {
        log::error << "Dual Pointcloud2 in xyzirt Mode is not supported yet!" << log::endl;
        return {};
    }

    auto message = ROSMessage::create<ROSMessageType::PointCloud2>();
    auto ros_message = reinterpret_cast<sensor_msgs::PointCloud2*>(message->ptr);

    message->topic_name = remapper->remap(topic_prefix + "point_cloud2");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    time::Timestamp ts = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.stamp = ros::Time(ts.tv_sec, ts.tv_nsec);
    ros_message->header.frame_id = frame_id;
    ros_message->height = 1;
    ros_message->width = data_impl->point_number;
    ros_message->is_bigendian = false;
    ros_message->is_dense = true;
    ros_message->point_step = sizeof(PointXYZIRT);
    ros_message->row_step = 0;
    ros_message->data.resize(sizeof(PointXYZIRT) * data_impl->point_number);

    {
        sensor_msgs::PointField field;
        field.name = "x";
        field.offset = 0;
        field.count = 1;
        field.datatype = sensor_msgs::PointField::FLOAT32;
        ros_message->fields.push_back(field);
    }
    {
        sensor_msgs::PointField field;
        field.name = "y";
        field.offset = 4;
        field.count = 1;
        field.datatype = sensor_msgs::PointField::FLOAT32;
        ros_message->fields.push_back(field);
    }
    {
        sensor_msgs::PointField field;
        field.name = "z";
        field.offset = 8;
        field.count = 1;
        field.datatype = sensor_msgs::PointField::FLOAT32;
        ros_message->fields.push_back(field);
    }
    {
        sensor_msgs::PointField field;
        field.name = "intensity";
        field.offset = 12;
        field.count = 1;
        field.datatype = sensor_msgs::PointField::FLOAT32;
        ros_message->fields.push_back(field);
    }
    {
        sensor_msgs::PointField field;
        field.name = "ring";
        field.offset = 16;
        field.count = 1;
        field.datatype = sensor_msgs::PointField::UINT16;
        ros_message->fields.push_back(field);
    }
    {
        sensor_msgs::PointField field;
        field.name = "time";
        field.offset = 18;
        field.count = 1;
        field.datatype = sensor_msgs::PointField::FLOAT32;
        ros_message->fields.push_back(field);
    }

    for (size_t index_pt = 0; index_pt < data_impl->point_number; ++index_pt) {
        auto ros_pt = reinterpret_cast<PointXYZIRT*>(ros_message->data.data());
        ros_pt += index_pt;

        ros_pt->x = data_impl->points[index_pt].x;
        ros_pt->y = data_impl->points[index_pt].y;
        ros_pt->z = data_impl->points[index_pt].z;
        ros_pt->intensity = data_impl->points[index_pt].intensity;
        ros_pt->ring = data_impl->points[index_pt].ring;
        ros_pt->time = data_impl->points[index_pt].time_offset;
    }

    ret.emplace_back(std::move(message));
    return ret;
};

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::Points>(device::data::SensorDataPtr& sensor_data,
                                                                               const std::string& topic_prefix,
                                                                               const std::string& frame_id,
                                                                               const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::Points*>(sensor_data.get());

    if (data_impl->meta.point_format == device::data::Points::PointFormat::XYZIRT) {
        return convertXYZIRTCloud(sensor_data, topic_prefix, frame_id, remapper);
    }

    std::vector<ROSMessagePtr> ret;

    size_t single_return_point_number = data_impl->point_number;
    if (data_impl->meta.return_type == device::data::Points::ReturnType::Dual) {
        single_return_point_number /= 2;
    }

    {
        auto message = ROSMessage::create<ROSMessageType::PointCloud2>();
        auto ros_message = reinterpret_cast<sensor_msgs::PointCloud2*>(message->ptr);

        message->topic_name = remapper->remap(topic_prefix + "point_cloud2");
        message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
        ros_message->header.seq = sensor_data->sequence;
        ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
        ros_message->header.frame_id = frame_id;

        pcl::PointCloud<pcl::PointXYZI> pcl_cloud;
        pcl_cloud.resize(single_return_point_number);
        float* dst_ptr = reinterpret_cast<float*>(pcl_cloud.points.data());
        float* src_ptr = reinterpret_cast<float*>(data_impl->points);
        const float* dst_ptr_end =
                dst_ptr + (size_t)(single_return_point_number) * (sizeof(pcl::PointXYZI) / sizeof(float));

        using Pt = device::data::Points::PointXYZCIDPAT;
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
    }

    if (data_impl->meta.return_type == device::data::Points::ReturnType::Dual) {
        auto message = ROSMessage::create<ROSMessageType::PointCloud2>();
        auto ros_message = reinterpret_cast<sensor_msgs::PointCloud2*>(message->ptr);

        message->topic_name = remapper->remap(topic_prefix + "point_cloud2_last");
        message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
        ros_message->header.seq = sensor_data->sequence;
        ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
        ros_message->header.frame_id = frame_id;

        pcl::PointCloud<pcl::PointXYZI> pcl_cloud;
        pcl_cloud.resize(single_return_point_number);
        float* dst_ptr = reinterpret_cast<float*>(pcl_cloud.points.data());
        float* src_ptr = reinterpret_cast<float*>(data_impl->points) +
                         (size_t)(single_return_point_number) * (sizeof(pcl::PointXYZI) / sizeof(float));
        const float* dst_ptr_end =
                dst_ptr + (size_t)(single_return_point_number) * (sizeof(pcl::PointXYZI) / sizeof(float));

        using Pt = device::data::Points::PointXYZCIDPAT;
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
    }

    return ret;
}

template<>
std::vector<ROSMessagePtr> ROSMessage::convert<device::SensorDataType::LaserScan>(
        device::data::SensorDataPtr& sensor_data,
        const std::string& topic_prefix,
        const std::string& frame_id,
        const common::Remapper* remapper)
{
    auto data_impl = reinterpret_cast<device::data::LaserScan*>(sensor_data.get());
    auto message = ROSMessage::create<ROSMessageType::LaserScan>();
    auto ros_message = reinterpret_cast<sensor_msgs::LaserScan*>(message->ptr);
    std::vector<ROSMessagePtr> ret;

    message->topic_name = remapper->remap(topic_prefix + "laser_scan");
    message->timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    ros_message->header.seq = sensor_data->sequence;
    ros_message->header.stamp = to_ros_time(sensor_data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id;

    ros_message->angle_min = data_impl->angle_min;
    ros_message->angle_max = data_impl->angle_max;
    ros_message->angle_increment = data_impl->angle_increment;

    ros_message->time_increment = data_impl->time_increment;
    ros_message->scan_time = data_impl->scan_time;

    ros_message->range_min = data_impl->range_min;
    ros_message->range_max = data_impl->range_max;

    ros_message->ranges.resize(data_impl->ranges_length);
    ros_message->intensities.resize(data_impl->intensities_length);
    memcpy(ros_message->ranges.data(), &data_impl->ranges_intensities_data, sizeof(float) * data_impl->ranges_length);
    memcpy(ros_message->intensities.data(),
           &data_impl->ranges_intensities_data[data_impl->ranges_length],
           sizeof(float) * data_impl->intensities_length);

    ret.emplace_back(std::move(message));
    return ret;
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz
