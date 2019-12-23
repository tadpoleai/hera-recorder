///
/// @file dummy_data.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Implementation of Display::parse for SensorData for Dummy
/// @date 2019-12-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "dummy_data.hpp"

#include "common/logger/logger.hpp"

namespace wayz {
namespace hera {

template<>
std::string DisplayData::parse<SensorDataType::Dummy>(std::vector<SensorDataPtr>&& sensor_datas, bool& is_jpeg)
{
    is_jpeg = false;
    std::string result = "";
    for (auto&& data : sensor_datas) {
        if (data->sensor_data_type == SensorDataType::Dummy) {
            log::debug << "Display: Parsing Dummy Data" << log::endl;
            auto data_impl = reinterpret_cast<dummy::DummySensorData*>(data.get());
            result += std::to_string(data_impl->sequence);
            result += " ";
            result += std::to_string(data_impl->int_value);
            result += "\n";
        }
    }
    return result;
}

}  // namespace hera
}  // namespace wayz
