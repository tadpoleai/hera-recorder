///
/// @file imu_data.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Implementation of Display::parse for SensorData for IMU
/// @date 2019-12-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "imu_data.hpp"

#include <iomanip>
#include <sstream>

#include "common/logger/logger.hpp"

namespace wayz {
namespace hera {

template<>
std::string DisplayData::parse<SensorDataType::ImuMagneticField>(std::vector<SensorDataPtr>&& sensor_datas,
                                                                 bool& is_jpeg)
{
    is_jpeg = false;
    std::stringstream result;
    for (auto&& data : sensor_datas) {
        if (data->sensor_data_type == SensorDataType::ImuMagneticField) {
            auto data_impl = reinterpret_cast<imu::ImuMagneticFieldSensorData*>(data.get());

            result << "GYRO: ";
            for (auto i = 0; i < 3; ++i) {
                result << " ";
                result << std::showpos << std::fixed << std::setprecision(4) << std::setw(8) << std::setfill(' ')
                       << data_impl->angular_velocity[i];
            }
            result << "\n";

            result << "ACC : ";
            for (auto i = 0; i < 3; ++i) {
                result << " ";
                result << std::showpos << std::fixed << std::setprecision(4) << std::setw(8) << std::setfill(' ')
                       << data_impl->linear_acceleration[i];
            }
            result << "\n";

            result << "MAG: ";
            for (auto i = 0; i < 3; ++i) {
                result << " ";
                result << std::showpos << std::fixed << std::setprecision(4) << std::setw(8) << std::setfill(' ')
                       << data_impl->magnetic_field[i];
            }
            result << "\n";
        }
    }
    return result.str();
}

}  // namespace hera
}  // namespace wayz
