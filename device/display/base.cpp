///
/// @file base.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-06-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <fstream>
#include <map>

#include "common/include/logger/logger.hpp"
#include "common/include/utils/time.hpp"
#include "display_data.hpp"
#include "sensor_data.hpp"

// First time include sensor_data_types
#include "sensor_data_types.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

// Include sensor_data_types 2nd time, to expand weak (default) template function of parse
#undef _HERA_DEVICE_SENSOR_DATA_TYPES_HPP_
#define SENSOR_DATA_TYPE_TEMPLATE_EXPAND

#undef SENSOR_DATA_TYPE_DEFINE
#define SENSOR_DATA_TYPE_DEFINE(name, value)                                                                         \
    template<>                                                                                                       \
    SingleDisplayData __attribute__((weak))                                                                          \
    SingleDisplayData::parse<SensorDataType::name>(std::vector<SensorDataPtr> && sensor_datas, const bool is_detail) \
    {                                                                                                                \
        return SingleDisplayData({.text_data = "parser<" #name "> not implemented"});                                \
    }

// Expanded here
#include "sensor_data_types.hpp"
// End of Expand

void DisplayData::update_from(std::vector<SensorDataPtr>&& sensor_datas, const bool is_detail)
{
    if (sensor_datas.size() == 0) {
        // log::warn << "DisplayParser: Sensor data size is 0" << log::endl;
        return;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    // Categorize sensor data
    std::map<SensorDataType, std::vector<SensorDataPtr>> categorized_datas;
    for (auto&& data : sensor_datas) {
        if (categorized_datas.count(data->sensor_data_type)) {
            categorized_datas[data->sensor_data_type].emplace_back(std::move(data));
        } else {
            categorized_datas[data->sensor_data_type] = {data};
        }
    }

    for (auto&& categorized_data : categorized_datas) {
        switch (categorized_data.first) {
#undef _HERA_DEVICE_SENSOR_DATA_TYPES_HPP_
#define SENSOR_DATA_TYPE_TEMPLATE_EXPAND

#undef SENSOR_DATA_TYPE_DEFINE
#define SENSOR_DATA_TYPE_DEFINE(name, value)                                                                   \
    case SensorDataType::name: {                                                                               \
        auto single_disp_data =                                                                                \
                SingleDisplayData::parse<SensorDataType::name>(std::move(categorized_data.second), is_detail); \
        if (!single_disp_data.text_data.empty() || !single_disp_data.jpeg_data.empty()) {                      \
            categorized_disp_data[categorized_data.first] = std::move(single_disp_data);                       \
        }                                                                                                      \
    } break;

            // Expanded here
#include "sensor_data_types.hpp"
            // End

        default:
            log::warn << "DisplayData: Unknown SensorDataType " << static_cast<int>(categorized_data.first)
                      << log::endl;
            break;
        }
    }
}

void DisplayData::clear_all()
{
    std::unique_lock<std::mutex> lock(mutex_);

    categorized_disp_data.clear();
}

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz