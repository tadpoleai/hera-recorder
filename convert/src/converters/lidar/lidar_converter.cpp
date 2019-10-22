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

LidarConverter::LidarConverter(const std::string& frame_id,
                               const std::string& device_name,
                               const std::string& device_data_folder) :
    Converter(frame_id, device_name, device_data_folder)
{
    pcl_topic_name_ = topic_name_prefix_ + "pointcloud2";
}

LidarConverter::~LidarConverter()
{
    if (thread_) {
        thread_->join();
    }
}

bool LidarConverter::convert_and_write_one_data(const std::shared_ptr<DeviceRawData>& rawdata)
{
    auto sensor_data = Lidar::do_convert(rawdata);
    if (!sensor_data) {
        return false;
    }

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

    sensor_msgs::PointCloud2 pcl_msg;
    pcl::toROSMsg(pcl_cloud, pcl_msg);

    pcl_msg.header.stamp = ros_time_intrinsic;
    pcl_msg.header.seq = sensor_data->sequence;
    pcl_msg.header.frame_id = frame_id_;

    bag_write_mutex_.lock();
    bag_->write(pcl_topic_name_, ros_time_intrinsic, pcl_msg);
    bag_write_mutex_.unlock();

    return true;
}


}  // namespace tron
}  // namespace wayz