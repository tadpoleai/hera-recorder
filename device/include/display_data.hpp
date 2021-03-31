///
/// @file device_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class DisplayData
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "types.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

class SensorData;

class DisplayData;

class SingleDisplayData;

///
/// @brief Shared pointer to DisplayData
///
using DisplayDataPtr = std::shared_ptr<DisplayData>;

///
/// @brief Final class display data
///
/// Which contains string or jpeg data to display for viewer
///
class DisplayData final {
public:
    using SensorDataPtr = std::shared_ptr<SensorData>;

public:
    DisplayData() = default;

    ///
    /// @brief Update displayData from a vector of uncategorized SensorData
    ///
    /// @param sensor_datas vector of SensorData, containing multi-SensorDataType data
    /// @param bool is_detail, show detail (i.e., high resolution)
    ///
    /// @note This function calls SingleDisplayData::parse()
    ///
    void update_from(std::vector<SensorDataPtr>&& sensor_datas, const bool is_detail = false);

    ///
    /// @brief Clear all display data
    ///
    void clear_all();

public:
    std::map<SensorDataType, SingleDisplayData> categorized_disp_data;

private:
    mutable std::mutex mutex_;
};

class SingleDisplayData final {
public:
    using SensorDataPtr = std::shared_ptr<SensorData>;

    ///
    /// @brief Convert Categorized SensorData to DisplayData
    ///
    /// @tparam T SensorDataType sensor data type
    /// @param sensor_datas vector of SensorData, must be same sensor data type<T>
    /// @note implementation is in folder: display/<T>_data.cpp
    template<SensorDataType T>
    static SingleDisplayData parse(std::vector<SensorDataPtr>&& sensor_datas, const bool is_detail = false);

public:
    SingleDisplayData() = default;

public:
    std::string text_data{};  ///< Text data, if exists
    std::string jpeg_data{};  ///< Jpeg data, if exists
};

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz