///
/// @file flir.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Flir and its Storage Datas
/// @version 0.1
/// @date 2019-11-07
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_FLIR
#include <flycapture/FlyCapture2.h>
#endif
#endif

#include "../../device.hpp"
#include "../camera_data.hpp"
#include "flir_timestamp_calculator.hpp"

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
    struct FlirCompressedImageUnion {
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
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        FlirCompressedImageUnion data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(FlirCompressedImageUnion)];  ///< union entry: raw buffer of bytes
    };
};

///
/// @brief Device data for Flir, raw, Derived from Storage Data
///
class FlirRawImage final : public data::DeviceData {
public:
    FlirRawImage() = delete;

public:
    struct FlirRawImageRawData {
        ///
        /// @brief timestamp of capture, UTC, in ns
        ///
        /// Timestamp at the half from capture triggered to shutter closed,
        /// i.e, trigger timestamp + 1/2 of shutter duration
        uint64_t timestamp_intrinsic_ns;
        data::RawImage::ImageMeta image_meta;  ///< meta data of image
        uint32_t flir_embedded_timstamp;       ///< flir embedded timstamp, see flir's manual
        uint32_t flir_embedded_shutter;        ///< flir embedded timstamp, see flir's manual
        uint32_t image_data_size;              ///< size of image_data
        uint8_t image_data[0];                 ///< raw image data
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        FlirRawImageRawData data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(FlirRawImageRawData)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

///
/// @brief Flir (former PointGrey) Camera, Derived from Device
///
class Flir final : public Device {
public:
    ///
    /// @brief Construct a new Flir object
    ///
    /// @note pass IpAddress as essential parameters
    /// @see Device::Device()
    Flir(const uint32_t id,
         const std::string& vendor_type,
         const std::string& name,
         storage::StorageManager* const storage) :
        Device(id, vendor_type, name, storage, HistoryDepth_, {DeviceParameterType::IpAddress})
    {}
    Flir(const Flir&) = delete;
    Flir& operator=(const Flir&) = delete;

    ///
    /// @brief Destroy the Flir object
    ///
    /// calls Device::stop()
    virtual ~Flir()
    {
        stop();
    }

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_FLIR
    virtual HeraErrno connect() override;

    virtual void disconnect() override;

    virtual data::DeviceDataPtr fetch() override;

    virtual HeraErrno adjust_parameter(DeviceParameterType type, const std::string& value) override;
#endif
#endif

    virtual data::SensorDataPtr convert(data::DeviceDataPtr& storage_data) override
    {
        return do_convert(storage_data);
    }

    ///
    /// @brief Static convert function for read / convert / replay
    ///
    static data::SensorDataPtr do_convert(data::DeviceDataPtr& storage_data);

private:
#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_FLIR
    ///
    /// @brief Flir error wrapper
    ///
    /// @param error Flir error type
    /// @see Flir's SDK
    /// @return HeraErrno HeraErrno::CanNotOpenCamera
    ///
    /// calls Device::handle_error()
    /// @note Call this function will set the Device into DeviceStatus::Error
    HeraErrno handle_flir_error(const FlyCapture2::Error& error);
#endif
#endif

private:
    static constexpr size_t HistoryDepth_ = 1;      ///< History Depth, 1
    static constexpr int32_t GrabTimeoutMs_ = 200;  ///< Timeout for grabbing an image data
    static constexpr int32_t NumBuffers_ = 5;       ///< Size of image buffer in FLIR's SDK

private:
    std::string ip_;  ///< IP address of camera

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_FLIR
    FlyCapture2::Camera camera_;                          ///< FLIR's SDK's Camera object
    FlyCapture2::PGRGuid guid_;                           ///< FLIR's SDK's G-UID object
    FlyCapture2::InterfaceType interface_type_;           ///< FLIR's SDK's InterfaceType object
    flir::FlirTimestampCalculator timestamp_calculator_;  ///< FLIR Camera's TimestampCalculator
#endif
#endif
};

}  // namespace flir
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz
