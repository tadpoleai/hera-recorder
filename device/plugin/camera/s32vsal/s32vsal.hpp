///
/// @file s32vsal.hpp
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
#include "S32vSAL/camera/vsdk.hpp"
#endif

#include "data/camera_data.hpp"
#include "device.hpp"

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

///
/// @brief S32VSal (former PointGrey) Camera, Derived from Device
///
class S32VSal final : public Device {
public:
    ///
    /// @brief Construct a new S32VSal object
    ///
    /// @note pass IpAddress as essential parameters
    /// @see Device::Device()
    S32VSal(const uint32_t id,
            const std::string& vendor_type,
            const std::string& name,
            const bool forward,
            ipc::IPCQueue<data::SensorData>* const ipc_queue,
            storage::StorageManager* const storage) :
        Device(id, vendor_type, name, forward, ipc_queue, storage, HistoryDepth_, EssentialParameterTypes),
        frame_rate_(0)
    {}
    S32VSal(const S32VSal&) = delete;
    S32VSal& operator=(const S32VSal&) = delete;

    static DevicePtr create(const uint32_t id,
                            const std::string& vendor_type,
                            const std::string& name,
                            const bool forward,
                            ipc::IPCQueue<data::SensorData>* const ipc_queue,
                            storage::StorageManager* const storage)
    {
        return std::make_unique<S32VSal>(id, vendor_type, name, forward, ipc_queue, storage);
    }

    ///
    /// @brief Destroy the S32VSal object
    ///
    /// calls Device::stop()
    virtual ~S32VSal()
    {
        stop();
    }

#ifdef WITH_DRIVER
    virtual HeraErrno connect() override;

    virtual void disconnect() override;

    virtual data::DeviceDataPtr fetch() override;

    virtual HeraErrno adjust_parameter(const std::string& type, const std::string& value) override;
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
    static const std::vector<std::string> EssentialParameterTypes;  ///< Essential Parameters for device

    static const std::vector<std::string> OptionalParameterTypes;  ///< Optional Parameters for device

private:
    static constexpr size_t HistoryDepth_ = 1;  ///< History Depth, 1
    static constexpr size_t ImageWidth_ = 1280;
    static constexpr size_t ImageHeight_ = 800;
    static constexpr size_t ImageMonoDataSize_ = ImageWidth_ * ImageHeight_;     ///< Mono, i.e., only Y channel
    static constexpr size_t ImageYUVDataSize_ = ImageWidth_ * ImageHeight_ * 2;  ///< YUV 4:2:2
    static constexpr size_t CsiPort_ = 0;

    uint32_t frame_rate_;
    time::Timestamp last_time_;

    bool compress_;
    double compress_quality_;

    uint8_t* frame_data_;

    std::mutex mutex_;

#ifdef WITH_DRIVER
    AppContext app_context_;
#endif
};

}  // namespace s32vsal
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz
