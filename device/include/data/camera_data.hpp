///
/// @file camera_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Derived classes of SensorData for Camera
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
/// @brief SensorData for Camera
///
/// Compatible with ROS CompressedImage
class CompressedImage final : public SensorData {
public:
    ///
    /// @brief Format of compression for CompressedImage
    ///
    enum class CompressFormat : uint8_t {
        PNG = 0,  ///< Portable Network Graphics
        JPEG = 1  ///< Joint Photographic Experts Group
    };

public:
    CompressFormat compress_format;  ///< compress format
    uint32_t image_data_size;        ///< size of image_data, in bytes
    uint8_t image_data[0];           ///< image data, compressed
};

///
/// @brief SensorData for Camera
///
/// Need minor effort to ROS Image
class Image final : public SensorData {
public:
    ///
    /// @brief Format of pixel for Image
    ///
    /// Compatible with FLIR's SDK
    enum class PixelFormat : uint32_t {
        // From FlyCapture SDK
        Mono8 = 0x80000000,         ///< 8 bits of mono information
        YUV8_411 = 0x40000000,      ///< YUV 4:1:1
        YUV8_422 = 0x20000000,      ///< YUV 4:2:2
        YUV8_444 = 0x10000000,      ///< YUV 4:4:4
        RGB8 = 0x08000000,          ///< R = G = B = 8 bits
        Mono16 = 0x04000000,        ///< 16 bits of mono information
        RGB16 = 0x02000000,         ///< R = G = B = 16 bits
        SignedMono16 = 0x01000000,  ///< 16 bits of signed mono information
        SignedRGB16 = 0x00800000,   ///< R = G = B = 16 bits signed
        Raw8 = 0x00400000,          ///< 8 bit raw data output of sensor
        Raw16 = 0x00200000,         ///< 16 bit raw data output of sensor
        Mono12 = 0x00100000,        ///< 12 bits of mono information
        Raw12 = 0x00080000,         ///< 12 bit raw data output of sensor
        BGR8 = 0x80000008,          ///< 24 bit BGR
        BGRU8 = 0x40000008,         ///< 32 bit BGRU
        RGBU8 = 0x40000002,         ///< 32 bit RGBU
        BGR16 = 0x02000001,         ///< R = G = B = 16 bits
        BGRU16 = 0x02000002,        ///< 64 bit BGRU
        Reserved = 0xFFFFFFF,       ///< Reserved
    };

    ///
    /// @brief Format of bayer tile for Image
    ///
    /// Compatible with FLIR's SDK
    enum class BayerFormat : uint32_t {
        NONE = 0,              ///< No bayer tile format
        RGGB,                  ///< Red-Green-Green-Blue
        GRBG,                  ///< Green-Red-Blue-Green
        GBRG,                  ///< Green-Blue-Red-Green
        BGGR,                  ///< Blue-Green-Green-Red
        Reserved = 0xFFFFFFF,  ///< Reserved
    };

    ///
    /// @brief Meta data for Image
    ///
    struct ImageMeta {
        uint32_t rows;             ///< number of rows, i.e., height
        uint32_t cols;             ///< number of rows, i.e., width
        uint32_t stride;           ///< pixel stride, i.e., number of bytes for a single pixel
        PixelFormat pixel_format;  ///< pixel format
        BayerFormat bayer_format;  ///< bayer tile format
    };

public:
    ImageMeta image_meta;      ///< meta of image
    uint32_t image_data_size;  ///< size of image_data, in bytes
    uint8_t image_data[0];     ///< image data in bytes buf
};

#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz