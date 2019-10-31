//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "lidar_converter.hpp"

#include <devices/src/lidar/lidar.hpp>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>
#include <sensor_msgs/PointCloud2.h>

namespace wayz {
namespace tron {

LidarConverter::LidarConverter(const std::string& device_type,
                               const std::string& device_name,
                               const std::string& device_data_folder,
                               const std::string& optional_frame_id,
                               const std::vector<std::string>& optional_topics,
                               ConverterHandler* handler) :
    Converter(device_type, device_name, device_data_folder, optional_frame_id, handler)
{
    if (optional_topics.size() > 0) {
        pcl_topic_name_ = optional_topics[0];
    } else {
        pcl_topic_name_ = topic_name_prefix_ + "point_cloud2";
    }
}

LidarConverter::~LidarConverter()
{
    if (thread_) {
        thread_->join();
    }
}

bool LidarConverter::convert_one_data(const std::shared_ptr<DeviceRawData>& raw_data)
{
    if (!raw_data) {
        sem_wait(managed_this_->sem_converter);
        managed_this_->data.msg_type = MsgType::Invalid;
        managed_this_->data.msg = nullptr;
        managed_this_->data.timestamp_ns = 0;
        managed_this_->data.topic_name = "";
        sem_post(managed_this_->sem_writer);
        return false;
    }

    auto sensor_data = Lidar::do_convert(raw_data);
    DataLidar* data_lidar_buf = reinterpret_cast<DataLidar*>(sensor_data->data_buf);
    auto ros_time_intrinsic = to_ros_time(sensor_data->timestamp_intrinsic_ns);

    pcl::PointCloud<pcl::PointXYZI> pcl_cloud;
    pcl_cloud.resize(data_lidar_buf->point_number);
    float* dst_ptr = reinterpret_cast<float*>(pcl_cloud.points.data());
    float* src_ptr = reinterpret_cast<float*>(data_lidar_buf->lidar_points);
    const float* dst_ptr_end = dst_ptr + (size_t)(data_lidar_buf->point_number) *
                                                 (sizeof(pcl::PointXYZI) / sizeof(float));
    for (; dst_ptr < dst_ptr_end;) {
        memcpy(dst_ptr, src_ptr, sizeof(LidarPoint));
        dst_ptr += sizeof(pcl::PointXYZI) / sizeof(float);
        src_ptr += sizeof(LidarPoint) / sizeof(float);
    }

    auto pcl_msg = new sensor_msgs::PointCloud2;
    pcl::toROSMsg(pcl_cloud, *pcl_msg);

    pcl_msg->header.stamp = ros_time_intrinsic;
    pcl_msg->header.seq = sensor_data->sequence;
    pcl_msg->header.frame_id = frame_id_;

    sem_wait(managed_this_->sem_converter);
    managed_this_->data.msg_type = MsgType::SensorMsgsPointCloud2;
    managed_this_->data.msg = reinterpret_cast<void*>(pcl_msg);
    managed_this_->data.timestamp_ns = sensor_data->timestamp_intrinsic_ns;
    managed_this_->data.topic_name = pcl_topic_name_;
    sem_post(managed_this_->sem_writer);

    return true;
}


}  // namespace tron
}  // namespace wayz