//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <string>

namespace wayz {
namespace tron {

enum class MsgType : int32_t {
    Invalid = -1,
    SensorMsgsImu = 0,
    SensorMsgsMagneticField,
    SensorMsgsPointCloud2,
    SensorMsgsCompressedImage,
};

}
}  // namespace wayz
