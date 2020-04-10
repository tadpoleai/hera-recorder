///
/// @file s32vmipi.cpp
/// @author chunchen.wang (chunchen.wang@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2020-04-03
///
/// Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "s32vmipi.hpp"

#ifdef WITH_DRIVER
#include <turbojpeg.h>
#endif

#include <common/include/logger/logger.hpp>

namespace wayz {
namespace hera {
namespace device {
namespace camera {
namespace s32vmipi {

const std::vector<DeviceParameterType> S32VMipi::EssentialParameterTypes = {};

const std::vector<DeviceParameterType> S32VMipi::OptionalParameterTypes = {};

auto _ = DeviceFactory::register_type({.type = DeviceVendorType::CameraS32VMipi,
                                       .type_name = "camera/s32vmipi",
                                       .create = &S32VMipi::create,
                                       .do_convert = &S32VMipi::do_convert,
                                       .essential_parameter_types = S32VMipi::EssentialParameterTypes,
                                       .optional_parameter_types = S32VMipi::OptionalParameterTypes,
#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VMIPI
                                       .implemented = true
#else
                                       .implemented = false
#endif
#else
                                       .implemented = false
#endif
});

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VMIPI

/*用户自定义回调函数*/
static void SeqEventCallBack(uint32_t aEventType, void* apUserVal)
{
    AppContext* lpAppContext = (AppContext*)apUserVal;
    FrameTimeS frameTime;
    if (lpAppContext) {
        if (aEventType == AY_EVENT_TYPE_FRAMEDONE) {
            VisionSdk_GetTimeStatus(frameTime);
            lpAppContext->mFrmDoneCnt++;
            /*printf("Frame done message arrived #%u . StartTime %lu . LastDoneTime %lu.\n",
                       lpAppContext->mFrmDoneCnt++,
                       frameTime.mStartTime,
                       frameTime.mLastDoneTime);*/

        }  // if frame done arrived
    }      // if user pointer is NULL
}  // SeqEventCallBack()

HeraErrno S32VMipi::connect()
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

void S32VMipi::disconnect()
{
    log::debug << "S32VMipi: Disconnect" << log::endl;
    VisionSdk_Release(app_context_);

    if (frame_data_ != nullptr) {
        free(frame_data_);
    }
}

///
/// @note U/V Channel is abandon
/// Only Y Channel is saved to storage
data::DeviceDataPtr S32VMipi::fetch()
{
    // Fetch rawdata from camera;
    uint64_t get_length = 0;
    if (VisionSdk_ReadFrame(app_context_, &frame_data_, &get_length) != AY_SUCCESS) {
        log::warn << "S32VMipi: Can not read frame, read aborted" << log::endl;
        return nullptr;
    }

    if (VisionSdk_Display(app_context_) != AY_SUCCESS) {
        log::warn << "S32VMipi: Can not display frame, read aborted" << log::endl;
        return nullptr;
    }

    // if (app_context_.mFrmCnt % FrameSkipStep_ != 0) {
    //     return nullptr;
    // }

    // Total length of device data
    auto length = sizeof(S32VMipiRawImage) + ImageMonoDataSize_;
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::CameraS32VMipi,
                                         DeviceDataType::CameraS32VMipiRawImage,
                                         sequence_++);
    auto derived_data = static_cast<S32VMipiRawImage*>(data.get());

    // Copy data
    derived_data->data.timestamp_capture_start_ns = time::Timestamp::now();
    derived_data->data.timestamp_capture_end_ns = derived_data->data.timestamp_capture_start_ns;
    derived_data->data.image_meta.cols = ImageWidth_;
    derived_data->data.image_meta.rows = ImageHeight_;
    derived_data->data.image_meta.stride = 1;
    derived_data->data.image_meta.pixel_format = data::Image::PixelFormat::Mono8;
    derived_data->data.image_meta.bayer_format = data::Image::BayerFormat::NONE;
    derived_data->data.image_data_size = ImageMonoDataSize_;

    uint8_t* dst_ptr = derived_data->data.image_data;
    uint8_t* src_ptr = frame_data_;
    const uint8_t* SrcPtrEnd = frame_data_ + ImageYUVDataSize_;
    while (src_ptr < SrcPtrEnd) {
        src_ptr++;
        *dst_ptr++ = *src_ptr++;
    }

    return data;
}

HeraErrno S32VMipi::adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    default:
        return HeraErrno::UnimplementedParameter;
    }
}

#endif
#endif

data::SensorDataPtr S32VMipi::do_convert(data::DeviceDataPtr& storage_data)
{
    if (storage_data->is_type(DeviceDataType::CameraS32VMipiRawImage)) {
        // Raw DeviceData of Derived Type
        auto raw_data = static_cast<S32VMipiRawImage*>(storage_data.get());

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
    } else {
        log::warn << "S32VMipi: Unknown DeviceData Format" << log::endl;
        return data::SensorData::broken_data();
    }
    return data::SensorData::broken_data();
}

}  // namespace s32vmipi
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz