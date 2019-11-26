#include "../processer.hpp"

namespace wayz {
namespace hera {
namespace convert {

template<>
void Processer::process<SensorDataType::NavSatFix>(SensorDataPtr& data)
{
    auto data_impl = reinterpret_cast<gnss::NavSatFixSensorData*>(data.get());
    auto message = ROSMessage::create<ROSMessageType::NavSatFix>();
    auto ros_message = reinterpret_cast<sensor_msgs::NavSatFix*>(message->ptr);
    message->topic_name = remap(topic_prefix_ + "nav_sat_fix");
    message->timestamp_ns = data->timestamp_intrinsic_ns;
    ros_message->header.seq = data->sequence;
    ros_message->header.stamp = to_ros_time(data->timestamp_intrinsic_ns);
    ros_message->header.frame_id = frame_id_;

    ros_message->status.status =
            static_cast<decltype(ros_message->status.status)>(data_impl->status.status);
    ros_message->status.service =
            static_cast<decltype(ros_message->status.service)>(data_impl->status.service);

    ros_message->latitude = data_impl->latitude;
    ros_message->longitude = data_impl->longitude;
    ros_message->altitude = data_impl->altitude;

    ros_message->position_covariance_type =
            static_cast<decltype(ros_message->position_covariance_type)>(
                    data_impl->position_covariance_type);

    for (size_t i = 0; i < 9; ++i) {
        ros_message->position_covariance[i] = data_impl->position_covariance[i];
    }

    publish(std::move(message));
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz