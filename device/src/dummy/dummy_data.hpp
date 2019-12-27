///
/// @file dummy_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Derived classes of SensorData for Dummy
/// @version 0.1
/// @date 2019-11-07
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "../device_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

#pragma pack(push, 1)

///
/// @brief Dummy SensorData, for sample
///
class Dummy final : public SensorData {
public:
    int32_t int_value;       ///< a normal value
    uint32_t string_length;  ///< length of variable-length buf string_buf
    uint8_t string_buf[0];   ///< a variable-length buf
};

#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz