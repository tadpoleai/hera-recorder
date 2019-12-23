#include <iostream>

#include "common/ipc/ipc_queue.hpp"
#include "common/logger/logger.hpp"
#include "devices/src/device_factory.hpp"

using namespace wayz::hera;

void data_handler(const uint32_t index, SensorDataPtr& data)
{
    if (data != nullptr) {
        auto logger = log::info << "Read Succeed: id = ";
        logger << index << " seq = " << data->sequence << " ";

        switch (data->sensor_data_type) {
        case SensorDataType::Dummy: {
            logger << "type = Dummy ";
            auto data_impl = reinterpret_cast<dummy::DummySensorData*>(data.get());
            logger << "value = " << data_impl->int_value;
            logger << log::endl;
            break;
        }
        case SensorDataType::NavSatFix: {
            logger << "type = NavSatFix ";
            auto data_impl = reinterpret_cast<gnss::NavSatFixSensorData*>(data.get());
            logger << "lat = " << data_impl->latitude << " ";
            logger << "lon = " << data_impl->longitude << " ";
            logger << "alt = " << data_impl->altitude << " ";
            logger << log::endl;
            break;
        }
        case SensorDataType::ImuMagneticField: {
            logger << "type = ImuMagneticField ";
            auto data_impl = reinterpret_cast<imu::ImuMagneticFieldSensorData*>(data.get());
            logger << "gyro_x = " << data_impl->angular_velocity[0] << " ";
            logger << log::endl;
            break;
        }
        case SensorDataType::PointsXYZI: {
            logger << "type = PointsXYZI ";
            auto data_impl = reinterpret_cast<lidar::PointsXYZISensorData*>(data.get());
            logger << "points = " << data_impl->point_number << " ";
            logger << log::endl;
            break;
        }
        case SensorDataType::CompressedImage: {
            logger << "type = CompressedImage ";
            auto data_impl = reinterpret_cast<camera::CompressedImageSensorData*>(data.get());
            logger << "image_size = " << data_impl->image_data_size << " ";
            logger << log::endl;
            break;
        }
        default: {
            logger << "type = Unknown ";
            logger << log::endl;
            break;
        }
        }
    }
}

int main()
{
    static constexpr auto DeviceNum = 6;
    std::vector<std::unique_ptr<ipc::IPCQueue<SensorData>>> ipc_queues;

    for (auto i = 0; i < DeviceNum; ++i) {
        auto ipc_queue = ipc::IPCQueue<SensorData>::create();
        ipc_queue->open(i, ipc::OpenMode::Read);
        ipc_queues.emplace_back(std::move(ipc_queue));
    }

    while (true) {
        for (auto i = 0; i < DeviceNum; ++i) {
            auto data = ipc_queues[i]->read();
            data_handler(i, data);
        }
    }
    return 0;
}