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
namespace s32vsal {

#pragma pack(push, 1)

///
/// @brief Device data for S32VSal, raw, Derived from Storage Data
///
class S32VSalRawImage final : public data::DeviceData {
public:
    S32VSalRawImage() = delete;

public:
    struct S32VSalRawData {
        ///
        /// @brief timestamp of capture start, UTC, in ns
        ///
        uint64_t timestamp_capture_start_ns;
        ///
        /// @brief timestamp of capture ebd, UTC, in ns
        ///
        /// @note not implenmented yet
        uint64_t timestamp_capture_end_ns;
        data::Image::ImageMeta image_meta;  ///< meta data of image
        uint32_t image_data_size;           ///< size of image_data
        uint8_t image_data[0];              ///< raw image data
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        S32VSalRawData data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(S32VSalRawData)];  ///< union entry: raw buffer of bytes
    };
};

class S32VSalCompressedImage final : public data::DeviceData {
public:
    S32VSalCompressedImage() = delete;

public:
    struct S32VSalCompressedData {
        ///
        /// @brief timestamp of capture start, UTC, in ns
        ///
        uint64_t timestamp_capture_start_ns;
        ///
        /// @brief timestamp of capture ebd, UTC, in ns
        ///
        /// @note not implenmented yet
        uint64_t timestamp_capture_end_ns;

        data::CompressedImage::CompressFormat compress_format;  ///< format of compression
        uint32_t image_data_size;                               ///< size of image_data, in bytes
        uint8_t image_data[0];                                  ///< compressed image data
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        S32VSalCompressedData data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(S32VSalCompressedData)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

}  // namespace s32vsal
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz