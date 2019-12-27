#include <iostream>

#include "common/ipc/ipc_queue.hpp"
#include "common/logger/logger.hpp"
#include "device/include.hpp"

using namespace wayz::hera;

void data_handler(device::data::SensorDataPtr& data)
{
    if (data != nullptr) {
        auto logger = log::info << "Read Succeed: id = ";
        logger << data->sensor_id << " seq = " << data->sequence << " ";

        switch (data->sensor_data_type) {
        case device::SensorDataType::Dummy: {
            logger << "type = Dummy ";
            auto data_impl = reinterpret_cast<device::data::Dummy*>(data.get());
            logger << "value = " << data_impl->int_value;
            logger << log::endl;
            break;
        }
        case device::SensorDataType::NavSatFix: {
            logger << "type = NavSatFix ";
            auto data_impl = reinterpret_cast<device::data::NavSatFix*>(data.get());
            logger << "lat = " << data_impl->latitude << " ";
            logger << "lon = " << data_impl->longitude << " ";
            logger << "alt = " << data_impl->altitude << " ";
            logger << log::endl;
            break;
        }
        case device::SensorDataType::ImuMagneticField: {
            logger << "type = ImuMagneticField ";
            auto data_impl = reinterpret_cast<device::data::ImuMagneticField*>(data.get());
            logger << "gyro_x = " << data_impl->angular_velocity[0] << " ";
            logger << log::endl;
            break;
        }
        case device::SensorDataType::PointsXYZI: {
            logger << "type = PointsXYZI ";
            auto data_impl = reinterpret_cast<device::data::PointsXYZI*>(data.get());
            logger << "points = " << data_impl->point_number << " ";
            logger << log::endl;
            break;
        }
        case device::SensorDataType::CompressedImage: {
            logger << "type = CompressedImage ";
            auto data_impl = reinterpret_cast<device::data::CompressedImage*>(data.get());
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
    auto ipc_queue = ipc::IPCQueue<device::data::SensorData>::create();
    ipc_queue->open(0, ipc::OpenMode::Read);

    while (true) {
        auto data = ipc_queue->read();
        if (data) {
            data_handler(data);
        }
    }
    return 0;
}