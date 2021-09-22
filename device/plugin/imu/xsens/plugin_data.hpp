///
/// @file plugin_data.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "xsens_defs.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace imu {
namespace xsens {

#pragma pack(push, 1)

///
/// @brief Device data for Aceinna 9-axis Imu, Derived from Storage Data
///
class XsensData final : public data::DeviceData {
public:
    XsensData() = delete;

public:
    bool is_synced;
    int64_t timestamp_intrinsic;
    MessageHeader message;
    // ....
    // Followed by message data;
};

#pragma pack(pop)

}  // namespace xsens
}  // namespace imu
}  // namespace device
}  // namespace hera
}  // namespace wayz