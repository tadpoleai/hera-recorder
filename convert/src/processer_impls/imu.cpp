#include <type_traits>

#include "../processer.hpp"

namespace wayz {
namespace hera {
namespace convert {

template<>
void Processer::process<SensorDataType::ImuMagneticField>(SensorDataPtr& data)
{
    auto data_impl = reinterpret_cast<imu::ImuMagneticFieldSensorData*>(data.get());
    {
        auto message = ROSMessage::create<ROSMessageType::Imu>();
        auto ros_message = reinterpret_cast<sensor_msgs::Imu*>(message->ptr);
        message->topic_name = remap(topic_prefix_ + "imu");
        message->timestamp_ns = data->timestamp_intrinsic_ns;
        ros_message->header.seq = data->sequence;
        ros_message->header.stamp = to_ros_time(data->timestamp_intrinsic_ns);
        ros_message->header.frame_id = frame_id_;

        ros_message->orientation_covariance[0] = -1;
        ros_message->linear_acceleration.x = data_impl->linear_acceleration[0];
        ros_message->linear_acceleration.y = data_impl->linear_acceleration[1];
        ros_message->linear_acceleration.z = data_impl->linear_acceleration[2];
        ros_message->angular_velocity.x = data_impl->angular_velocity[0];
        ros_message->angular_velocity.y = data_impl->angular_velocity[1];
        ros_message->angular_velocity.z = data_impl->angular_velocity[2];

        publish(std::move(message));
    }

    {
        auto message = ROSMessage::create<ROSMessageType::MagneticField>();
        auto ros_message = reinterpret_cast<sensor_msgs::MagneticField*>(message->ptr);
        message->topic_name = remap(topic_prefix_ + "magnetic_field");
        message->timestamp_ns = data_impl->timestamp_intrinsic_ns;
        ros_message->header.seq = data_impl->sequence;
        ros_message->header.stamp = to_ros_time(data->timestamp_intrinsic_ns);
        ros_message->header.frame_id = frame_id_;

        ros_message->magnetic_field.x = data_impl->magnetic_field[0];
        ros_message->magnetic_field.y = data_impl->magnetic_field[1];
        ros_message->magnetic_field.z = data_impl->magnetic_field[2];

        publish(std::move(message));
    }
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz