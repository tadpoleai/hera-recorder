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
#include "display_data.hpp"
#include "sensor_data_types.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

template<>
SingleDisplayData SingleDisplayData::parse<SensorDataType::InsBestPosition>(std::vector<SensorDataPtr>&& sensor_datas, const bool is_detail)
{
    SingleDisplayData result;
    auto data_impl = reinterpret_cast<data::BestPosition*>(sensor_datas[0].get());

    result.text_data += "S: " + std::to_string(data_impl->solution_status) + "\n";
    result.text_data += "P: " + std::to_string(data_impl->position_type) + "\n";
    result.text_data += "LATI: " + std::to_string(data_impl->latitude) + "\n";
    result.text_data += "LONG: " + std::to_string(data_impl->longitude) + "\n";
    result.text_data += "ALTI: " + std::to_string(data_impl->altitude) + "\n";
    result.text_data += "LATD: " + std::to_string(data_impl->latitude_deviation) + "\n";
    result.text_data += "LOND: " + std::to_string(data_impl->longitude_deviation) + "\n";
    result.text_data += "ALTD: " + std::to_string(data_impl->altitude_deviation) + "\n";

    return result;

}  // namespace data

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
