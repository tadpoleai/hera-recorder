///
/// @file imu_data.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Implementation of Display::parse for SensorData for IMU
/// @date 2019-12-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "data/imu_data.hpp"

#include <iomanip>
#include <sstream>

#include "common/include/logger/logger.hpp"
#include "display_data.hpp"
#include "sensor_data_types.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

template<>
SingleDisplayData SingleDisplayData::parse<SensorDataType::ImuMagneticField>(std::vector<SensorDataPtr>&& sensor_datas)
{
    std::stringstream result;
    auto data_impl = reinterpret_cast<data::ImuMagneticField*>(sensor_datas[0].get());

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

    return SingleDisplayData({.text_data = result.str()});
}

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
