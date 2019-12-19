///
/// @file processer.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Processor's main methods
/// @version 0.1
/// @date 2019-12-19
///
/// @copyright Copyright (c) 2019
///

#include "processer.hpp"

namespace wayz {
namespace hera {
namespace replay {

/// Init semaphores, open device and create thread
///
Processer::Processer(const uint32_t id,
                     const std::string& type,
                     const std::string& vendor,
                     const std::string& name,
                     const std::string& folder) :
    publish_sem(new sem_t),
    receive_sem(new sem_t),
    message(std::move(SensorData::end_of_file())),
    device_(DeviceFactory::create(id, type + "/" + vendor, name, folder, true))
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
void Processer::publish(SensorDataPtr&& in_message)
{
    sem_wait(publish_sem);
    message = std::move(in_message);
    sem_post(receive_sem);
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

        if (data->sensor_data_type == SensorDataType::EndOfFile) {
            publish(std::move(data));
            break;
        } else {
            if (data->sensor_data_type != SensorDataType::Broken) {
                publish(std::move(data));
            } else {
                log::warn << "Replayer: got broken data" << log::endl;
            }
        }
    }
}
}  // namespace replay
}  // namespace hera
}  // namespace wayz