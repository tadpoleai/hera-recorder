///
/// @file plugin_entry.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <turbojpeg.h>

#include <common/include/logger/logger.hpp>

#include "data/camera_data.hpp"
#include "plugin_common.hpp"
#include "plugin_param.hpp"

#ifdef WITH_DRIVER
#include "S32vSAL/camera/vsdk.hpp"
#endif

namespace wayz {
namespace hera {
namespace device {
namespace camera {
namespace s32vsal {

///
/// @brief S32VSal (former PointGrey) Camera, Derived from Device
///
HERA_PLUGIN_DEFINE_START("camera/s32vsal", 0x0402, 1)

#include "plugin_data.hpp"

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

static constexpr size_t ImageWidth_ = 1280;
static constexpr size_t ImageHeight_ = 800;

static constexpr size_t ImageMonoDataSize_ = ImageWidth_ * ImageHeight_;     ///< Mono, i.e., only Y channel
static constexpr size_t ImageYUVDataSize_ = ImageWidth_ * ImageHeight_ * 2;  ///< YUV 4:2:2
static constexpr size_t CsiPort_ = 0;

time::Timestamp last_time_;
uint8_t* frame_data_;
std::mutex mutex_;
AppContext app_context_;
#endif

HERA_PLUGIN_DEFINE_END

#ifdef WITH_DRIVER
FrameSyncTime frameSyncTimes;

/*用户自定义回调函数*/
static void SeqEventCallBack(uint32_t aEventType, void* apUserVal)
{
    AppContext* lpAppContext = (AppContext*)apUserVal;
    FrameSyncTime mFrameSyncTimes;
    if (lpAppContext) {
        if (aEventType == AY_EVENT_TYPE_FRAMEDONE) {
            if (!VisionSdk_GetFrameSyncTimeStatus(mFrameSyncTimes)) {
                memcpy(&frameSyncTimes, &mFrameSyncTimes, sizeof(FrameSyncTime));
            }
            lpAppContext->mFrmDoneCnt++;
        }  // if frame done arrived
    }      // if user pointer is NULL
}  // SeqEventCallBack()

HeraErrno DevicePlugin::connect()
{
    app_context_.mError = false;
    app_context_.mFrmCnt = 0;
    app_context_.mFrmDoneCnt = 0;
    app_context_.mWidth = ImageWidth_;
    app_context_.mHeight = ImageHeight_;
    app_context_.sCsiPort = CsiPort_;

    frame_data_ = (uint8_t*)malloc(ImageYUVDataSize_);
    if (frame_data_ == nullptr) {
        return handle_error(HeraErrno::CanNotOpenCamera, "Out of memory");
    }

    if (VisionSdk_Initialize(app_context_) != AY_SUCCESS) {
        return handle_error(HeraErrno::CanNotOpenCamera, "VisionSdk_Initialize failed");
    }

    if (VisionSdk_LibsPrepare(app_context_, SeqEventCallBack) != AY_SUCCESS) {
        return handle_error(HeraErrno::CanNotOpenCamera, "VisionSdk_LibsPrepare failed");
    }

    return HeraErrno::Success;
}

void DevicePlugin::disconnect()
{
    log::debug << "S32VSal: Disconnect" << log::endl;
    VisionSdk_Release(app_context_);

    if (frame_data_ != nullptr) {
        free(frame_data_);
    }
}

///
/// @note U/V Channel is abandon
/// Only Y Channel is saved to storage
data::DeviceDataPtr DevicePlugin::fetch()
{
    if (local_parameters_.get_SkipFrame()) {
        auto now = time::Timestamp::now();                                             // Get time now
        const int64_t PeriodNs = time::OneSecond / local_parameters_.get_FrameRate();  // Period of Frame Rate [ns]
        const int64_t last_count = last_time_.tv_nsec / PeriodNs;
        const int64_t now_count = now.tv_nsec / PeriodNs;
        last_time_ = now;

        if (now_count == last_count) {  // Skip
            usleep(1000);
            return nullptr;
        }
    }

    // Fetch rawdata from camera;
    uint64_t get_length = 0;
    constexpr uint32_t msTimeout = 10;
    if (VisionSdk_ReadFrame(app_context_, &frame_data_, &get_length, msTimeout) != AY_SUCCESS) {
        log::warn << "S32VSal: Can not read frame, read aborted" << log::endl;
        return nullptr;
    }
    // VisionSdk_Display();
    if (VisionSDK_FramePush(app_context_) != AY_SUCCESS) {
        log::warn << "S32VSal: Can not push frame, read aborted" << log::endl;
        return nullptr;
    }
    if (frameSyncTimes.mFrameSyncTime == 0)
        return nullptr;

    if (local_parameters_.get_SaveFormat() == +SaveFormat::RGB8) {
        // Total length of device data
        auto length = sizeof(S32VSalRawImage) + ImageMonoDataSize_;
        auto data = S32VSalRawImage::create(length, id_, sequence_++);
        auto derived_data = static_cast<S32VSalRawImage*>(data.get());

        // Copy data
        derived_data->data.timestamp_capture_start_ns = frameSyncTimes.mFrameSyncTime;
        derived_data->data.timestamp_capture_end_ns = frameSyncTimes.mLastDoneTime;
        derived_data->data.image_meta.cols = ImageWidth_;
        derived_data->data.image_meta.rows = ImageHeight_;
        derived_data->data.image_meta.stride = 1;
        derived_data->data.image_meta.pixel_format = data::Image::PixelFormat::Mono8;
        derived_data->data.image_meta.bayer_format = data::Image::BayerFormat::NONE;
        derived_data->data.image_data_size = ImageMonoDataSize_;
        log::debug << "Camrea/s32vsal frame count:" << frameSyncTimes.mFrmDoneCnt
                   << " , timestamp_capture_start_ns: " << derived_data->data.timestamp_capture_start_ns
                   << " , timestamp_capture_end_ns: " << derived_data->data.timestamp_capture_end_ns
                   << " ,delta_ns: " << frameSyncTimes.delta_ns << log::endl;
        uint8_t* dst_ptr = derived_data->data.image_data;
        uint8_t* src_ptr = frame_data_;
        const uint8_t* SrcPtrEnd = frame_data_ + ImageYUVDataSize_;
        while (src_ptr < SrcPtrEnd) {
            src_ptr++;
            *dst_ptr++ = *src_ptr++;
        }

        return data;
    } else {
        auto tj_instance = tjInitCompress();
        if (!tj_instance) {
            log::warn << "CameraS32VSal: " << get_name() << " can not compress, data abandoned" << log::endl;
            return nullptr;
        }
        uint8_t* jpeg_image = nullptr;
        size_t jpeg_image_size = 0;

        uint8_t* gray_image = new uint8_t[ImageMonoDataSize_];
        uint8_t* dst_ptr = gray_image;
        uint8_t* src_ptr = frame_data_;
        const uint8_t* SrcPtrEnd = frame_data_ + ImageYUVDataSize_;
        while (src_ptr < SrcPtrEnd) {
            /// @note Warning: Code below should only be used when image size is multiple of 8
            *dst_ptr++ = *(++src_ptr)++;
            *dst_ptr++ = *(++src_ptr)++;
            *dst_ptr++ = *(++src_ptr)++;
            *dst_ptr++ = *(++src_ptr)++;

            *dst_ptr++ = *(++src_ptr)++;
            *dst_ptr++ = *(++src_ptr)++;
            *dst_ptr++ = *(++src_ptr)++;
            *dst_ptr++ = *(++src_ptr)++;
        }

        /// @todo Check source image format
        /// @todo Add parameter for Sampling and Quality
        tjCompress2(tj_instance,
                    gray_image,
                    ImageWidth_,
                    0,
                    ImageHeight_,
                    TJPF_GRAY,                            // Source Image Format
                    &jpeg_image,                          // Output Image Pointer
                    &jpeg_image_size,                     // Output Image Size
                    TJSAMP_GRAY,                          // YUV Binning
                    local_parameters_.get_JpegQuality(),  // Quality
                    TJFLAG_FASTDCT | TJXOPT_GRAY);

        // Total length of device data
        auto length = sizeof(S32VSalCompressedImage) + jpeg_image_size;
        auto data = S32VSalCompressedImage::create(length, id_, sequence_++);
        auto derived_data = static_cast<S32VSalCompressedImage*>(data.get());

        // Copy data
        derived_data->data.timestamp_capture_start_ns = frameSyncTimes.mFrameSyncTime;
        derived_data->data.timestamp_capture_end_ns = frameSyncTimes.mLastDoneTime;
        derived_data->data.compress_format = data::CompressedImage::CompressFormat::JPEG;
        derived_data->data.image_data_size = jpeg_image_size;
        memcpy(derived_data->data.image_data, jpeg_image, jpeg_image_size);
        log::debug << "Camrea/s32vsal frame count:" << frameSyncTimes.mFrmDoneCnt
                   << " , timestamp_capture_start_ns: " << derived_data->data.timestamp_capture_start_ns
                   << " , timestamp_capture_end_ns: " << derived_data->data.timestamp_capture_end_ns
                   << " ,delta_ns: " << frameSyncTimes.delta_ns << log::endl;
        tjDestroy(tj_instance);
        tjFree(jpeg_image);
        delete[] gray_image;

        return data;
    }
}
#endif

data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameters)
{
    if (storage_data->is_type(S32VSalRawImage::TypeVal)) {
        // Raw DeviceData of Derived Type
        auto raw_data = static_cast<S32VSalRawImage*>(storage_data.get());

        // Create a SensorData from DeviceData
        uint32_t image_data_size = raw_data->data.image_data_size;
        uint32_t length = sizeof(data::Image) + image_data_size;
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::Image, length);
        auto camera_sensor_data = static_cast<data::Image*>(sensor_data.get());

        // Parse Data
        camera_sensor_data->timestamp_intrinsic_ns =
                raw_data->data.timestamp_capture_start_ns / 2 + raw_data->data.timestamp_capture_end_ns / 2;
        camera_sensor_data->image_meta = raw_data->data.image_meta;
        camera_sensor_data->image_data_size = image_data_size;
        memcpy(camera_sensor_data->image_data, raw_data->data.image_data, image_data_size);
        return sensor_data;
    } else if (storage_data->is_type(S32VSalCompressedImage::TypeVal)) {
        // Raw DeviceData of Derived Type
        auto raw_data = static_cast<S32VSalCompressedImage*>(storage_data.get());

        // Create a SensorData from DeviceData
        uint32_t image_data_size = raw_data->data.image_data_size;
        uint32_t length = sizeof(data::CompressedImage) + image_data_size;
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::CompressedImage, length);
        auto camera_sensor_data = static_cast<data::CompressedImage*>(sensor_data.get());

        // Parse Data
        camera_sensor_data->timestamp_intrinsic_ns =
                raw_data->data.timestamp_capture_start_ns / 2 + raw_data->data.timestamp_capture_end_ns / 2;
        camera_sensor_data->compress_format = raw_data->data.compress_format;
        camera_sensor_data->image_data_size = image_data_size;
        memcpy(camera_sensor_data->image_data, &(raw_data->data.image_data), image_data_size);
        return sensor_data;
    } else {
        log::warn << "S32VSal: Unknown DeviceData Format" << log::endl;
        return data::SensorData::broken_data();
    }
    return data::SensorData::broken_data();
}

}  // namespace s32vsal
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz