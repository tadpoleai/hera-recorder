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

#include <flycapture/FlyCapture2.h>

#include "../../device.hpp"
#include "../camera_data.hpp"
#include "flir_timestamp_calculator.hpp"

namespace wayz {
namespace hera {
namespace camera {

#pragma pack(push, 1)

///
/// @brief Storage data for Flir, compressed, Derived from Storage Data
///
class FlirCompressedStorageData final : public StorageData {
public:
    FlirCompressedStorageData() = delete;

public:
    struct FlirCompressedRawData {
        ///
        /// @brief timestamp of capture, UTC, in ns
        ///
        /// Timestamp at the half from capture triggered to shutter closed,
        /// i.e, trigger timestamp + 1/2 of shutter duration
        uint64_t timestamp_intrinsic_ns;

        CompressFormat compress_format;  ///< format of compression
        uint32_t image_data_size;        ///< size of image_data, in bytes
        uint8_t image_data[0];           ///< compressed image data
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        FlirCompressedRawData data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(FlirCompressedRawData)];  ///< union entry: raw buffer of bytes
    };
};

///
/// @brief Storage data for Flir, raw, Derived from Storage Data
///
class FlirRawStorageData final : public StorageData {
public:
    FlirRawStorageData() = delete;

public:
    struct FlirRawRawData {
        ///
        /// @brief timestamp of capture, UTC, in ns
        ///
        /// Timestamp at the half from capture triggered to shutter closed,
        /// i.e, trigger timestamp + 1/2 of shutter duration
        uint64_t timestamp_intrinsic_ns;
        ImageMeta image_meta;             ///< meta data of image
        uint32_t flir_embedded_timstamp;  ///< flir embedded timstamp, see flir's manual
        uint32_t flir_embedded_shutter;   ///< flir embedded timstamp, see flir's manual
        uint32_t image_data_size;         ///< size of image_data
        uint8_t image_data[0];            ///< raw image data
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        FlirRawRawData data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(FlirRawRawData)];  ///< union entry: raw buffer of bytes
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
    Flir(DeviceIdType id, const std::string& type, const std::string& name, const std::string& folder, bool read_mode) :
        Device(id, type, name, folder, read_mode, HistoryDepth_, {DeviceParameterType::IpAddress})
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

    virtual HeraErrno connect() override;

    virtual void disconnect() override;

    virtual StorageDataPtr fetch() override;

    virtual SensorDataPtr convert(StorageDataPtr& storage_data) override;

    virtual HeraErrno adjust_parameter(DeviceParameterType type, const std::string& value) override;

private:
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

private:
    static constexpr size_t HistoryDepth_ = 1;      ///< History Depth, 1
    static constexpr int32_t GrabTimeoutMs_ = 200;  ///< Timeout for grabbing an image data
    static constexpr int32_t NumBuffers_ = 5;       ///< Size of image buffer in FLIR's SDK

private:
    std::string ip_;  ///< IP address of camera

    FlyCapture2::Camera camera_;                          ///< FLIR's SDK's Camera object
    FlyCapture2::PGRGuid guid_;                           ///< FLIR's SDK's G-UID object
    FlyCapture2::InterfaceType interface_type_;           ///< FLIR's SDK's InterfaceType object
    flir::FlirTimestampCalculator timestamp_calculator_;  ///< FLIR Camera's TimestampCalculator
};

}  // namespace camera
}  // namespace hera
}  // namespace wayz