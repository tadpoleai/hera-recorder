///
/// @file processer_core.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Processor's main methods
/// @version 0.1
/// @date 2019-11-13
///
/// @copyright Copyright (c) 2019
///

#include "processer.hpp"

namespace wayz {
namespace hera {
namespace convert {

/// Init semaphores, open device and create thread
///
Processer::Processer(const std::string& type,
                     const std::string& vendor,
                     const std::string& name,
                     const std::string& folder,
                     const Remapper* remapper) :
    publish_sem(new sem_t),
    receive_sem(new sem_t),
    message(std::move(ROSMessage::create<ROSMessageType::BrokenData>())),
    remapper_(remapper),
    frame_id_(remap(type + "_" + name + "_link")),
    topic_prefix_("/" + type + "/" + name + "/"),
    device_(DeviceFactory::create(0, type + "/" + vendor, name, folder, true))
{
    sem_init(publish_sem, 0, 1);
    sem_init(receive_sem, 0, 0);
    thread_ = std::thread(&Processer::thread_function, this);
}

/// Wait for thread and then destory semaphores
///
Processer::~Processer()
{
    thread_.join();
    sem_destroy(publish_sem);
    sem_destroy(receive_sem);
}

/// Wait publish_sem and move from in_message to message,
/// then notify receiver by post receive_sem
void Processer::publish(ROSMessagePtr&& in_message)
{
    sem_wait(publish_sem);
    message = std::move(in_message);
    sem_post(receive_sem);
}

ros::Time Processer::to_ros_time(Timestamp ts)
{
    return ros::Time(ts.tv_sec, ts.tv_nsec);
}

std::string Processer::remap(std::string&& str)
{
    if (remapper_) {
        return remapper_->remap(std::forward<std::string>(str));
    } else {
        return str;
    }
}

/// Read data from device continuously, until end of file.
/// Calls process()
/// @see process()
void Processer::thread_function()
{
    while (true) {
        decltype(device_->read()) data = nullptr;
        if (device_ != nullptr) {
            data = device_->read();
        }

        if (data == nullptr) {
            publish(ROSMessage::create<ROSMessageType::EndOfFile>());
            break;
        } else {
            switch (data->sensor_data_type) {
            case SensorDataType::Broken:
                publish(ROSMessage::create<ROSMessageType::BrokenData>());
                break;
            case SensorDataType::ImuMagneticField:
                process<SensorDataType::ImuMagneticField>(data);
                break;
            case SensorDataType::PointsXYZI:
                process<SensorDataType::PointsXYZI>(data);
                break;
            case SensorDataType::CompressedImage:
                process<SensorDataType::CompressedImage>(data);
                break;
            case SensorDataType::NavSatFix:
                process<SensorDataType::NavSatFix>(data);
                break;
            default:
                log::error << "Converter:: Invalid Sensor Data Type" << log::endl;
                publish(ROSMessage::create<ROSMessageType::BrokenData>());
                break;
            }
        }
    }
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz