///
/// @file gnss_data.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Implementation of Display::parse for SensorData for GNSS
/// @date 2019-12-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "data/gnss_data.hpp"

#include "common/include/logger/logger.hpp"
#include "display_data.hpp"
#include "sensor_data_types.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

template<>
SingleDisplayData SingleDisplayData::parse<SensorDataType::NavSatFix>(std::vector<SensorDataPtr>&& sensor_datas)
{
    SingleDisplayData result;

    auto data_impl = reinterpret_cast<data::NavSatFix*>(sensor_datas[0].get());

    result.text_data += "LAT : " + std::to_string(data_impl->latitude) + "\n";
    result.text_data += "LON : " + std::to_string(data_impl->longitude) + "\n";
    result.text_data += "ALT : " + std::to_string(data_impl->altitude) + "\n";
    result.text_data += "STAT: " + (data_impl->status.status == NavSatFix::StatusType::NO_Fix) ? "NO FIX\n" : "FIXED\n";

    return result;
}

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
