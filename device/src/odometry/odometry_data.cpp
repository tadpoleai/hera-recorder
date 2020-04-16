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

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
