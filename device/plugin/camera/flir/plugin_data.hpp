///
/// @file plugin_data.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "data/camera_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace camera {
namespace flir {

#pragma pack(push, 1)

///
/// @brief Device data for Flir, compressed, Derived from Storage Data
///
class FlirCompressedImage final : public data::DeviceData {
public:
    FlirCompressedImage() = delete;

public:
    ///
    /// @brief timestamp of capture, UTC, in ns
    ///
    /// Timestamp at the half from capture triggered to shutter closed,
    /// i.e, trigger timestamp + 1/2 of shutter duration
    uint64_t timestamp_intrinsic_ns;

    data::CompressedImage::CompressFormat compress_format;  ///< format of compression
    uint32_t image_data_size;                               ///< size of image_data, in bytes
    uint8_t image_data[0];                                  ///< compressed image data
};

///
/// @brief Device data for Flir, raw, Derived from Storage Data
///
class FlirRawImage final : public data::DeviceData {
public:
    FlirRawImage() = delete;

public:
    ///
    /// @brief timestamp of capture, UTC, in ns
    ///
    /// Timestamp at the half from capture triggered to shutter closed,
    /// i.e, trigger timestamp + 1/2 of shutter duration
    uint64_t timestamp_intrinsic_ns;
    data::Image::ImageMeta image_meta;  ///< meta data of image
    uint32_t flir_embedded_timestamp;   ///< flir embedded timstamp, see flir's manual
    uint32_t flir_embedded_shutter;     ///< flir embedded timstamp, see flir's manual
    uint32_t image_data_size;           ///< size of image_data
    uint8_t image_data[0];              ///< raw image data
};

#pragma pack(pop)

}  // namespace flir
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz