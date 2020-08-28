///
/// @file plugin_data.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "data/odometry_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace aehaitai {

#pragma pack(push, 1)

enum class FrameType : uint8_t { ENCODER_ANGLE = 0x01u };

struct EncoderAngle {
    uint8_t angle_h8;
    uint8_t angle_l8;
    uint8_t direction;
};

///
/// @brief Device data for AEHaitaiData, Derived from Storage Data
///
class AEHaitaiData final : public data::DeviceData {
public:
    AEHaitaiData() = delete;

public:
    struct AEHaitaiDataUnion {
        uint8_t header;
        uint8_t address;
        FrameType frame_type;
        uint8_t frame_data[0];
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        AEHaitaiDataUnion data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(AEHaitaiDataUnion)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

}  // namespace aehaitai
}  // namespace odometry
}  // namespace device
}  // namespace hera
}  // namespace wayz