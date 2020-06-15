///
/// @file ins_data.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Implementation of Display::parse for SensorData for INS
/// @date 2020-06-03
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "data/ins_data.hpp"

#include "common/include/logger/logger.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

template<>
std::string DisplayData::parse<SensorDataType::InsBestPosition>(std::vector<SensorDataPtr>&& sensor_datas,
                                                                bool& is_jpeg)
{
    is_jpeg = false;
    std::string result;
    for (auto&& data : sensor_datas) {
        if (data->sensor_data_type == SensorDataType::InsBestPosition) {
            auto data_impl = reinterpret_cast<data::BestPosition*>(data.get());

            result += "S: " + std::to_string(data_impl->solution_status) + "\n";
            result += "P: " + std::to_string(data_impl->position_type) + "\n";
            result += "LATI: " + std::to_string(data_impl->latitude) + "\n";
            result += "LONG: " + std::to_string(data_impl->longitude) + "\n";
            result += "ALTI: " + std::to_string(data_impl->altitude) + "\n";
            result += "LATD: " + std::to_string(data_impl->latitude_deviation) + "\n";
            result += "LOND: " + std::to_string(data_impl->longitude_deviation) + "\n";
            result += "ALTD: " + std::to_string(data_impl->altitude_deviation) + "\n";
        }
    }
    return result;
}

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
