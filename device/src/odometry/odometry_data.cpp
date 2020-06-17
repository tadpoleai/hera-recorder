///
/// @file odometry_data.cpp
/// @author chunchen.wang (chunchen.wang@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2020-04-08
///
/// Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "data/odometry_data.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>

#include "common/include/logger/logger.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

template<>
std::string DisplayData::parse<SensorDataType::OdometryRearWheelSpeed>(std::vector<SensorDataPtr>&& sensor_datas,
                                                                       bool& is_jpeg)
{
    return "Driver is not implenmented";
}

template<>
std::string DisplayData::parse<SensorDataType::OdometrySteeringAngle>(std::vector<SensorDataPtr>&& sensor_datas,
                                                                      bool& is_jpeg)
{
    return "Driver is not implenmented";
}


template<>
std::string DisplayData::parse<SensorDataType::OdometryOrientation>(std::vector<SensorDataPtr>&& sensor_datas,
                                                                    bool& is_jpeg)
{
    is_jpeg = false;
    std::string result;
    for (auto&& data : sensor_datas) {
        if (data->sensor_data_type == SensorDataType::OdometryOrientation) {
            auto data_impl = reinterpret_cast<data::Orientation*>(data.get());
            result += "O: " + std::to_string(data_impl->orientation) + "rad\n";
            auto deg = data_impl->orientation / M_PI * 180.0;
            if (deg < 0) {
                deg += 360;
            }
            result += "O: " + std::to_string(deg) + "°\n";
        }
    }
    return result;
}

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
