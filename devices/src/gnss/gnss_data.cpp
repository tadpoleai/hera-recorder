///
/// @file gnss_data.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Implementation of Display::parse for SensorData for GNSS
/// @date 2019-12-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "gnss_data.hpp"

#include "common/logger/logger.hpp"

namespace wayz {
namespace hera {

template<>
std::string DisplayData::parse<SensorDataType::NavSatFix>(std::vector<SensorDataPtr>&& sensor_datas, bool& is_jpeg)
{
    is_jpeg = false;
    std::string result;
    for (auto&& data : sensor_datas) {
        if (data->sensor_data_type == SensorDataType::NavSatFix) {
            auto data_impl = reinterpret_cast<gnss::NavSatFixSensorData*>(data.get());

            result += "LAT : " + std::to_string(data_impl->latitude) + "\n";
            result += "LON : " + std::to_string(data_impl->longitude) + "\n";
            result += "ALT : " + std::to_string(data_impl->altitude) + "\n";
            result += "STAT: " + (data_impl->status.status == gnss::StatusType::NO_Fix) ? "NO FIX\n" : "FIXED\n";
        }
    }
    return result;
}

}  // namespace hera
}  // namespace wayz
