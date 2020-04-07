///
/// @file s32vmipi.hpp
/// @author chunchen.wang (chunchen.wang@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2020-04-03
///
/// Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#pragma once

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VMIPI
#include "vsdk.hpp"
#endif
#endif

#include "../../device.hpp"
#include "../camera_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace camera {
namespace s32vmipi {

#pragma pack(push, 1)

///
/// @brief Device data for S32VMipi, raw, Derived from Storage Data
///
class S32VMipiRawImage final : public data::DeviceData {
public:
    S32VMipiRawImage() = delete;

public:
    struct S32VMipiRawData {
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
        S32VMipiRawData data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(S32VMipiRawData)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

///
/// @brief S32VMipi (former PointGrey) Camera, Derived from Device
///
class S32VMipi final : public Device {
public:
    ///
    /// @brief Construct a new S32VMipi object
    ///
    /// @note pass IpAddress as essential parameters
    /// @see Device::Device()
    S32VMipi(const uint32_t id,
             const std::string& vendor_type,
             const std::string& name,
             const bool forward,
             ipc::IPCQueue<data::SensorData>* const ipc_queue,
             storage::StorageManager* const storage) :
        Device(id, vendor_type, name, forward, ipc_queue, storage, HistoryDepth_, EssentialParameterTypes)
    {}
    S32VMipi(const S32VMipi&) = delete;
    S32VMipi& operator=(const S32VMipi&) = delete;

    ///
    /// @brief Destroy the S32VMipi object
    ///
    /// calls Device::stop()
    virtual ~S32VMipi()
    {
        stop();
    }

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VMIPI
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

public:
    static const std::vector<DeviceParameterType> EssentialParameterTypes;  ///< Essential Parameters for device

    static const std::vector<DeviceParameterType> OptionalParameterTypes;  ///< Optional Parameters for device

private:
    static constexpr size_t HistoryDepth_ = 1;  ///< History Depth, 1
    static constexpr size_t ImageWidth_ = 1280;
    static constexpr size_t ImageHeight_ = 720;
    static constexpr size_t ImageMonoDataSize_ = ImageWidth_ * ImageHeight_;     ///< Mono, i.e., only Y channel
    static constexpr size_t ImageYUVDataSize_ = ImageWidth_ * ImageHeight_ * 2;  ///< YUV 4:2:2
    static constexpr size_t CsiPort_ = 0;
    static constexpr size_t FrameSkipStep_ = 30;

    uint8_t* frame_data_;

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VMIPI
    AppContext app_context_;

#endif
#endif
};

}  // namespace s32vmipi
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz
