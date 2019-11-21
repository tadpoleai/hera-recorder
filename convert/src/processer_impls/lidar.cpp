#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>

#include "../processer.hpp"

namespace wayz {
namespace hera {
namespace convert {

using Pts = lidar::PointsXYZISensorData::PointXYZI;

template<>
void Processer::process<SensorDataType::PointsXYZI>(SensorDataPtr& data)
{
    auto data_impl = reinterpret_cast<lidar::PointsXYZISensorData*>(data.get());
    auto message = ROSMessage::create<ROSMessageType::PointCloud2>();
    auto ros_message = reinterpret_cast<sensor_msgs::PointCloud2*>(message->ptr);
    message->topic_name = remap(topic_prefix_ + "point_cloud2");
    message->timestamp_ns = data->timestamp_intrinsic_ns;

    pcl::PointCloud<pcl::PointXYZI> pcl_cloud;
    pcl_cloud.resize(data_impl->point_number);
    float* dst_ptr = reinterpret_cast<float*>(pcl_cloud.points.data());
    float* src_ptr = reinterpret_cast<float*>(data_impl->points);
    const float* dst_ptr_end =
            dst_ptr + (size_t)(data_impl->point_number) * (sizeof(pcl::PointXYZI) / sizeof(float));

    for (; dst_ptr < dst_ptr_end;) {
        memcpy(dst_ptr, src_ptr, sizeof(Pts));
        dst_ptr += sizeof(pcl::PointXYZI) / sizeof(float);
        src_ptr += sizeof(Pts) / sizeof(float);
    }
    pcl::toROSMsg(pcl_cloud, *ros_message);
    ros_message->header.seq = data->sequence;
    ros_message->header.stamp = to_ros_time(data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id_;

    publish(std::move(message));
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz