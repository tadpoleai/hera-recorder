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
#include "display_data.hpp"
#include "sensor_data_types.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

template<>
SingleDisplayData SingleDisplayData::parse<SensorDataType::OdometryOrientation>(std::vector<SensorDataPtr>&& sensor_datas)
{
    SingleDisplayData result;

    auto data_impl = reinterpret_cast<data::Orientation*>(sensor_datas[0].get());
    result.text_data += "O: " + std::to_string(data_impl->orientation) + "rad\n";
    auto deg = data_impl->orientation / M_PI * 180.0;
    if (deg < 0) {
        deg += 360;
    }
    result.text_data += "O: " + std::to_string(deg) + "°\n";

    return result;
}

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
