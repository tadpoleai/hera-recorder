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
SingleDisplayData SingleDisplayData::parse<SensorDataType::ImuComposed>(std::vector<SensorDataPtr>&& sensor_datas,
                                                                        const bool is_detail)
{
    std::stringstream result;
    auto data_impl = reinterpret_cast<data::ImuComposed*>(sensor_datas[0].get());

    for (const auto& sensor_data: sensor_datas)
    {
        auto data_impl_temp = reinterpret_cast<data::ImuComposed*>(sensor_data.get());
        if (data_impl_temp->have_magnetic_field) {
            data_impl = data_impl_temp;
        }
    }

    if (data_impl->have_temperature) {
        result << "TEMP: " << data_impl->temperature << "\n";
    }

    if (data_impl->have_baro_pressure) {
        result << "BARO: " << data_impl->baro_pressure << "\n";
    }

    if (data_impl->have_orientation) {
        result << "QUAT:";
        for (auto i = 0; i < 4; ++i) {
            result << " ";
            result << std::showpos << std::fixed << std::setprecision(4) << std::setw(8) << std::setfill(' ')
                   << data_impl->orientation[i];
        }
        result << "\n";
    }

    if (data_impl->have_linear_acceleration) {
        result << "ACC :";
        for (auto i = 0; i < 3; ++i) {
            result << " ";
            result << std::showpos << std::fixed << std::setprecision(4) << std::setw(8) << std::setfill(' ')
                   << data_impl->linear_acceleration[i];
        }
        result << "\n";
    }

    if (data_impl->have_angular_velocity) {
        result << "GYRO:";
        for (auto i = 0; i < 3; ++i) {
            result << " ";
            result << std::showpos << std::fixed << std::setprecision(4) << std::setw(8) << std::setfill(' ')
                   << data_impl->angular_velocity[i];
        }
        result << "\n";
    }

    if (data_impl->have_magnetic_field) {
        result << "MAG :";
        for (auto i = 0; i < 3; ++i) {
            result << " ";
            result << std::showpos << std::fixed << std::setprecision(4) << std::setw(8) << std::setfill(' ')
                   << data_impl->magnetic_field[i];
        }
        result << "\n";
    }

    return SingleDisplayData({.text_data = result.str()});
}

template<>
SingleDisplayData SingleDisplayData::parse<SensorDataType::ImuMagneticField>(std::vector<SensorDataPtr>&& sensor_datas,
                                                                             const bool is_detail)
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
