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
SingleDisplayData SingleDisplayData::parse<SensorDataType::NavSatFix>(std::vector<SensorDataPtr>&& sensor_datas, const bool is_detail)
{
    SingleDisplayData result;

    auto data_impl = reinterpret_cast<data::NavSatFix*>(sensor_datas[0].get());

    if (data_impl->status.status == NavSatFix::StatusType::NO_Fix) {
        result.text_data += "LAT: --------\n";
        result.text_data += "LON: --------\n";
        result.text_data += "ALT: --------\n";
        result.text_data += "SAT: --------\n";
    } else {
        result.text_data += "LAT: " + std::to_string(data_impl->latitude) + "\n";
        result.text_data += "LON: " + std::to_string(data_impl->longitude) + "\n";
        result.text_data += "ALT: " + std::to_string(data_impl->altitude) + "\n";
        result.text_data += "SAT: " + std::to_string(data_impl->num_satellites) + "\n";
    }

    switch (data_impl->status.status)
    {
    case NavSatFix::StatusType::NO_Fix:
        result.text_data += "No Solution";
        break;
    
    case NavSatFix::StatusType::FIX:
        result.text_data += "Single Point";
        break;

    case NavSatFix::StatusType::SBAS_Fix:
        result.text_data += "Floating RTK";
        break;

    case NavSatFix::StatusType::GBAS_FIX:
        result.text_data += "Fixed RTK";
        break;
    
    default:
        result.text_data += "Unknown Status";
        break;
    }

    return result;
}


}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
