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

#include "plugin_common.hpp"
#include "plugin_data.hpp"
#include "plugin_param.hpp"

#ifdef WITH_DRIVER
#include <arpa/inet.h>
#include <flycapture/FlyCapture2.h>
#include <opencv2/opencv.hpp>

#include "device/driver/rawimg/color_correction.hpp"
#include "flir_timestamp_calculator.hpp"
#include "flir_timestamp_calculator.inc.cpp"
#endif

namespace wayz {
namespace hera {
namespace device {
namespace camera {
namespace flir {

///
/// @brief Flir (former PointGrey) Camera, Derived from Device
///
HERA_PLUGIN_DEFINE_START(1)

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

HeraErrno handle_flir_error(const FlyCapture2::Error& error, const std::string& extra = {});

HeraErrno set_property(FlyCapture2::PropertyType type, bool auto_set, float value);

HeraErrno set_range_auto_exposure();

HeraErrno set_range_auto_shutter();

HeraErrno set_range_auto_gain();

bool test_nvm_availability();

bool commit_nvm();

HeraErrno save_cc_parameter(const driver::ColorCorrectionParameter& ccm);

HeraErrno load_cc_parameter(driver::ColorCorrectionParameter& cc_param);

static constexpr int32_t GrabTimeoutMs_ = 200;  ///< Timeout for grabbing an image data
static constexpr int32_t NumBuffers_ = 5;       ///< Size of image buffer in FLIR's SDK

static constexpr double ExposureGranularity = 1;  ///< @todo
static constexpr double ShutterTimeGranularity23S6CMode7 = 0.027506;
static constexpr double ShutterTimeGranularity23S6CMode4 = 0.019206;
static constexpr double GainGranularity = 0.1;

static constexpr int32_t NVMControlRegister = 0x1240;  ///< Address of Flir NVM Control Registerin
static constexpr int32_t NVMOffsetRegister = 0x1244;   ///< Address of Flir Register in which NVM Offset stored
static constexpr uint32_t CCMagic = 0x4441'7A9F;       ///< Magic Number of Flir CC Parameteri

FlyCapture2::Camera camera_;                                       ///< FLIR's SDK's Camera object
FlyCapture2::PGRGuid guid_;                                        ///< FLIR's SDK's G-UID object
flir::FlirTimestampCalculator timestamp_calculator_;               ///< FLIR Camera's TimestampCalculator
std::unique_ptr<driver::ColorCorrecterBGR16> correcter_{nullptr};  ///< Color Correcter for Raw Image -> JPEG
#endif

HERA_PLUGIN_DEFINE_END

HERA_PLUGIN_EXPORT(CameraFlir, "camera/flir");

#ifdef WITH_DRIVER

/// Convert IP address to uint32_t(little-endian),
/// after that, use FLIR's SDK BusManager to connect.
///
/// Set SDK into buffer mode, and set grabbing timeout
HeraErrno DevicePlugin::connect()
{
    uint32_t ip_int;
    ip_int = ntohl(inet_addr(local_parameters_.get_IpAddress().c_str()));

    // Connect to Camera
    FlyCapture2::Error error;
    FlyCapture2::BusManager bus_manager;
    FlyCapture2::IPAddress flycapture_ipv4(ip_int);
    error = bus_manager.GetCameraFromIPAddress(flycapture_ipv4, &guid_);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }
    error = camera_.Connect(&guid_);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }

    // Get Camera Info
    FlyCapture2::CameraInfo camera_info;
    error = camera_.GetCameraInfo(&camera_info);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }
    log::info << "Flir: " << get_name() << " serial is " << camera_info.serialNumber << log::endl;

    // Set Driver to Buffer Mode
    FlyCapture2::FC2Config config;
    error = camera_.GetConfiguration(&config);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }
    config.grabTimeout = GrabTimeoutMs_;
    config.numBuffers = NumBuffers_;
    config.grabMode = FlyCapture2::BUFFER_FRAMES;
    if (local_parameters_.get_HighPerf()) {
        config.highPerformanceRetrieveBuffer = true;
        log::info << "Flir: " << get_name() << " enable high-performance-retrieve-buffer, parameters are immutable"
                  << log::endl;
    }
    error = camera_.SetConfiguration(&config);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }

    // Parse Mode and Format
    FlyCapture2::Mode mode = FlyCapture2::MODE_0;
    FlyCapture2::PixelFormat pixel_format = FlyCapture2::PIXEL_FORMAT_RAW8;

    switch (local_parameters_.get_Binning()) {
    case Binning::B11:
        mode = FlyCapture2::MODE_7;
        break;
    case Binning::B22:
        mode = FlyCapture2::MODE_4;
        break;
    }
    switch (local_parameters_.get_Format()) {
    case Format::MONO8:
        pixel_format = FlyCapture2::PIXEL_FORMAT_MONO8;
        break;
    case Format::MONO12:
        pixel_format = FlyCapture2::PIXEL_FORMAT_MONO12;
        break;
    case Format::RAW8:
        pixel_format = FlyCapture2::PIXEL_FORMAT_RAW8;
        break;
    case Format::RAW12:
        pixel_format = FlyCapture2::PIXEL_FORMAT_RAW12;
        break;
    }

    // Ask if Mode is supported
    FlyCapture2::Format7Info format7_info;
    bool supported = false;
    format7_info.mode = mode;
    error = camera_.GetFormat7Info(&format7_info, &supported);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }
    // Set Mode and Format
    FlyCapture2::Format7ImageSettings format7_image_settings;
    format7_image_settings.mode = mode;
    format7_image_settings.pixelFormat = pixel_format;
    format7_image_settings.width = format7_info.maxWidth;
    format7_image_settings.height = format7_info.maxHeight;
    FlyCapture2::Format7PacketInfo format7_packet_info;
    bool is_valid;
    error = camera_.ValidateFormat7Settings(&format7_image_settings, &is_valid, &format7_packet_info);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }
    if (!is_valid) {
        return handle_error(HeraErrno::CanNotOpenCamera, "Flir: Format7 Setting is not valid");
    }

    error = camera_.SetFormat7Configuration(&format7_image_settings, format7_packet_info.recommendedBytesPerPacket);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }

    // Set Image Size and ROI
    error = camera_.GetFormat7Info(&format7_info, &supported);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }
    format7_image_settings.width = format7_info.maxWidth;
    format7_image_settings.height = format7_info.maxHeight;
    // ROI
    uint32_t roi_width = format7_info.maxWidth;
    uint32_t roi_height = format7_info.maxHeight;
    uint32_t roi_offset_x = 0;
    uint32_t roi_offset_y = 0;
    switch (local_parameters_.get_Crop()) {
    case Crop::NONE:
        break;
    case Crop::CENTER:
        roi_width = local_parameters_.get_ROI_W();
        roi_height = local_parameters_.get_ROI_H();
        roi_offset_x = (static_cast<int32_t>(format7_info.maxWidth) - local_parameters_.get_ROI_W()) / 2;
        roi_offset_y = (static_cast<int32_t>(format7_info.maxHeight) - local_parameters_.get_ROI_H()) / 2;
        // fallthrough
    case Crop::RECT:
        roi_offset_x = local_parameters_.get_ROI_W();
        roi_offset_y = local_parameters_.get_ROI_H();
        break;
    }
    roi_width = roi_width / format7_info.imageHStepSize * format7_info.imageHStepSize;
    if (roi_width > format7_info.maxWidth) {
        roi_width = format7_info.maxWidth;
    }
    format7_image_settings.width = roi_width;
    roi_height = roi_height / format7_info.imageVStepSize * format7_info.imageVStepSize;
    if (roi_height > format7_info.maxHeight) {
        roi_height = format7_info.maxHeight;
    }
    format7_image_settings.height = roi_height;
    roi_offset_x = roi_offset_x / format7_info.offsetHStepSize * format7_info.offsetHStepSize;
    if (roi_offset_x > format7_info.maxWidth - roi_width) {
        roi_offset_x = format7_info.maxWidth - roi_width;
    }
    format7_image_settings.offsetX = roi_offset_x;
    roi_offset_y = roi_offset_y / format7_info.offsetVStepSize * format7_info.offsetVStepSize;
    if (roi_offset_y > format7_info.maxHeight - roi_height) {
        roi_offset_y = format7_info.maxHeight - roi_height;
    }
    format7_image_settings.offsetY = roi_offset_y;

    error = camera_.SetFormat7Configuration(&format7_image_settings, format7_packet_info.recommendedBytesPerPacket);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }

    // Set Exposure
    set_property(FlyCapture2::AUTO_EXPOSURE, local_parameters_.get_AutoExposure(), local_parameters_.get_Exposure());

    // Set Shutter
    set_property(FlyCapture2::SHUTTER, local_parameters_.get_AutoShutter(), local_parameters_.get_Shutter());

    // Set Gain
    set_property(FlyCapture2::GAIN, local_parameters_.get_AutoGain(), local_parameters_.get_Gain());

    // Set AutoExposureRange
    set_range_auto_exposure();

    // Set AutoShutterRange
    set_range_auto_shutter();

    // Set AutoGainRange
    set_range_auto_gain();

    // Color Correction
    if (!local_parameters_.get_SaveRaw()) {
        driver::ColorCorrectionParameter cc_param;
        if (local_parameters_.get_Correction() == +Correction::ONESHOT ||
            local_parameters_.get_Correction() == +Correction::STORE) {

            cc_param.dark_level[0] = (float)local_parameters_.get_DarkB();
            cc_param.dark_level[1] = (float)local_parameters_.get_DarkG();
            cc_param.dark_level[2] = (float)local_parameters_.get_DarkR();
            cc_param.white_balance[0] = (float)local_parameters_.get_BalanceB();
            cc_param.white_balance[1] = (float)local_parameters_.get_BalanceG();
            cc_param.white_balance[2] = (float)local_parameters_.get_BalanceR();
            cc_param.color_correction_matrix[0] = (float)local_parameters_.get_CCM11();
            cc_param.color_correction_matrix[1] = (float)local_parameters_.get_CCM12();
            cc_param.color_correction_matrix[2] = (float)local_parameters_.get_CCM13();
            cc_param.color_correction_matrix[3] = (float)local_parameters_.get_CCM21();
            cc_param.color_correction_matrix[4] = (float)local_parameters_.get_CCM22();
            cc_param.color_correction_matrix[5] = (float)local_parameters_.get_CCM23();
            cc_param.color_correction_matrix[6] = (float)local_parameters_.get_CCM31();
            cc_param.color_correction_matrix[7] = (float)local_parameters_.get_CCM32();
            cc_param.color_correction_matrix[8] = (float)local_parameters_.get_CCM33();
        }

        if (local_parameters_.get_Format() == +Format::RAW8 || local_parameters_.get_Format() == +Format::RAW12) {
            if (local_parameters_.get_Correction() == +Correction::STORE) {
                auto error = save_cc_parameter(cc_param);
                if (error != HeraErrno::OK) {
                    return error;
                }
            } else if (local_parameters_.get_Correction() == +Correction::LOAD) {
                auto error = load_cc_parameter(cc_param);
                if (error != HeraErrno::OK) {
                    return error;
                }
            }
        }

        log::debug << "Flir: " << get_name() << " CC Parameter:\n" << cc_param << log::endl;
        correcter_ = std::make_unique<driver::ColorCorrecterBGR16>(cc_param,
                                                                   local_parameters_.get_Gamma(),
                                                                   local_parameters_.get_ColorTemp());
    }


    // Start Capture
    error = camera_.StartCapture();
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }

    return HeraErrno::Success;
}

/// Call Flir SDK StopCapture() and Disconnect()
///
void DevicePlugin::disconnect()
{
    log::debug << "Flir:: disconnecting" << log::endl;
    camera_.StopCapture();
    camera_.Disconnect();
    log::debug << "Flir:: disconnected" << log::endl;
}

/// Calculate time::Timestamp by time::Timestamp_calculator and check synced.
/// Then convert fetched image into RGB and use libjpeg_turbo to
/// compress to jpeg format
/// @todo Add a parameter of storage format, CompressedImage or RawImage
/// @note Compression took ~10ms per frame sized 1280x720 on i5-8550U
data::DeviceDataPtr DevicePlugin::fetch()
{
    // Fetch rawdata from camera;
    FlyCapture2::Image raw_image;
    auto error = camera_.RetrieveBuffer(&raw_image);

    if (error != FlyCapture2::PGRERROR_OK) {
        log::warn << "Flir: " << get_name() << " failed to retrieve due to " << error.GetDescription() << log::endl;
        return nullptr;
    }

    // Check sync
    auto now = time::Timestamp::now();
    int64_t timestamp_intrinsic_ns;
    auto metadata = raw_image.GetMetadata();
    bool synced = timestamp_calculator_.get_intrinsic_time(
            // Calculate timestamp
            timestamp_intrinsic_ns,
            now,
            flir::FlirEmbeddedTimestamp(metadata.embeddedTimeStamp),
            flir::FlirEmbeddedShutter(metadata.embeddedShutter));
    if (!synced) {
        log::warn << "Flir: " << get_name() << " not synced" << log::endl;
        if (local_parameters_.get_NeedSync()) {
            return nullptr;
        } else {
            timestamp_intrinsic_ns = now;
        }
    }

    auto rows = raw_image.GetRows();
    auto cols = raw_image.GetCols();

    if (local_parameters_.get_SaveRaw()) {
        auto raw_image_size = raw_image.GetDataSize();
        // Total length of device data
        auto length = sizeof(FlirRawImage) + raw_image_size;
        auto data = data::DeviceData::create(length,
                                             id_,
                                             DeviceVendorType::CameraFlir,
                                             DeviceDataType::CameraFlirRawImage,
                                             sequence_++);
        auto derived_data = static_cast<FlirRawImage*>(data.get());

        // Copy data
        derived_data->timestamp_intrinsic_ns = timestamp_intrinsic_ns;

        derived_data->image_meta.rows = rows;
        derived_data->image_meta.cols = cols;
        derived_data->image_meta.stride = raw_image.GetStride();

        derived_data->image_meta.bayer_format = static_cast<data::Image::BayerFormat>(raw_image.GetBayerTileFormat());
        derived_data->image_meta.pixel_format = data::Image::PixelFormat(raw_image.GetPixelFormat());

        derived_data->flir_embedded_timestamp = metadata.embeddedTimeStamp;
        derived_data->flir_embedded_shutter = metadata.embeddedShutter;

        derived_data->image_data_size = raw_image_size;
        memcpy(derived_data->image_data, raw_image.GetData(), raw_image_size);

        return data;
    }

    // auto t0 = time::Timestamp::now();
    // Convert to 16bpp and Debayer
    FlyCapture2::Image converted_image;
    error = raw_image.Convert(FlyCapture2::PIXEL_FORMAT_RAW16, &converted_image);
    if (error != FlyCapture2::PGRERROR_OK) {
        log::warn << "Flir: " << get_name() << " can not convert, data abandoned" << log::endl;
        return nullptr;
    }

    int32_t debayer_code;
    switch (raw_image.GetBayerTileFormat()) {
    case FlyCapture2::RGGB:
        debayer_code = cv::COLOR_BayerRG2RGB_EA;
        break;
    case FlyCapture2::GRBG:
        debayer_code = cv::COLOR_BayerGR2RGB_EA;
        break;
    case FlyCapture2::GBRG:
        debayer_code = cv::COLOR_BayerGB2RGB_EA;
        break;
    case FlyCapture2::BGGR:
        debayer_code = cv::COLOR_BayerBG2RGB_EA;
        break;
    default:
        debayer_code = cv::COLOR_BayerRG2RGB_EA;
        break;
    }
    cv::Mat mat_bayer(rows, cols, CV_16UC1, converted_image.GetData());
    cv::Mat mat_rgb16(rows, cols, CV_16UC3);
    cv::cvtColor(mat_bayer, mat_rgb16, debayer_code);

    // auto t1 = time::Timestamp::now();
    // Do color correction
    const size_t ImagePixelNum = rows * cols;
    uint8_t* corrected_data = new uint8_t[3 * ImagePixelNum];
    correcter_->process(mat_rgb16.data, corrected_data, ImagePixelNum);

    // auto t2 = time::Timestamp::now();
    // Start Compression
    auto tj_instance = tjInitCompress();
    if (!tj_instance) {
        log::warn << "Flir: " << get_name() << " can not compress, data abandoned" << log::endl;
        delete[] corrected_data;
        return nullptr;
    }
    uint8_t* jpeg_image = nullptr;
    size_t jpeg_image_size = 0;

    auto tjsamp = TJSAMP_420;
    switch (local_parameters_.get_JpegSamp()) {
    case JpegSamp::S444:
        tjsamp = TJSAMP_444;
        break;
    case JpegSamp::S422:
        tjsamp = TJSAMP_422;
        break;
    case JpegSamp::S420:
        tjsamp = TJSAMP_420;
        break;
    case JpegSamp::SGRAY:
        tjsamp = TJSAMP_GRAY;
        break;
    case JpegSamp::S440:
        tjsamp = TJSAMP_440;
        break;
    case JpegSamp::S411:
        tjsamp = TJSAMP_411;
        break;
    }

    /// @todo Check source image format
    tjCompress2(tj_instance,
                corrected_data,
                cols,
                0,
                rows,
                TJPF_BGR,                             // Source Image Format
                &jpeg_image,                          // Output Image Pointer
                &jpeg_image_size,                     // Output Image Size
                tjsamp,                               // YUV Binning
                local_parameters_.get_JpegQuality(),  // Quality
                TJFLAG_FASTDCT);

    // Total length of device data
    auto length = sizeof(FlirCompressedImage) + jpeg_image_size;
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::CameraFlir,
                                         DeviceDataType::CameraFlirCompressedImage,
                                         sequence_++);
    auto derived_data = static_cast<FlirCompressedImage*>(data.get());

    // Copy data
    derived_data->timestamp_intrinsic_ns = timestamp_intrinsic_ns;
    derived_data->compress_format = data::CompressedImage::CompressFormat::JPEG;
    derived_data->image_data_size = jpeg_image_size;
    memcpy(derived_data->image_data, jpeg_image, jpeg_image_size);

    tjDestroy(tj_instance);
    tjFree(jpeg_image);
    delete[] corrected_data;

    // auto t3 = time::Timestamp::now();

    // log::debug << "Time: cvt: " << t1 - t0 << ", cc: " << t2 - t1 << ", comp: " << t3 - t2 << ", total: " << t3 - t1
    //            << log::endl;

    return data;
}

/// @todo Add muttable parameter for EV, Shutter Range, Brightneess, etc.
/// @todo Add immutable parameter for FPS (Need to pass parameter to Tron Sync Board)
HeraErrno DevicePlugin::adjust_parameter(const std::string& type, const std::string& value)
{
    if (type == "Exposure" || type == "AutoExposure") {
        return set_property(FlyCapture2::AUTO_EXPOSURE,
                            local_parameters_.get_AutoExposure(),
                            local_parameters_.get_Exposure());
    } else if (type == "Shutter" || type == "AutoShutter") {
        return set_property(FlyCapture2::SHUTTER, local_parameters_.get_AutoShutter(), local_parameters_.get_Shutter());
    } else if (type == "Gain" || type == "AutoGain") {
        return set_property(FlyCapture2::GAIN, local_parameters_.get_AutoGain(), local_parameters_.get_Gain());
    } else if (type == "MinExposure" || type == "MaxExposure") {
        return set_range_auto_exposure();
    } else if (type == "MinShutter" || type == "MaxShutter") {
        return set_range_auto_shutter();
    } else if (type == "MinGain" || type == "MaxGain") {
        return set_range_auto_gain();
    } else if (type == "ColorTemp") {
        correcter_->adjust_color_temp(local_parameters_.get_ColorTemp());
        return HeraErrno::OK;
    }

    return HeraErrno::OK;
}

HeraErrno DevicePlugin::set_property(FlyCapture2::PropertyType type, bool auto_set, float value)
{
    FlyCapture2::PropertyInfo property_info;
    property_info.type = type;
    FlyCapture2::Error error = camera_.GetPropertyInfo(&property_info);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_flir_error(error);
    }

    if (!property_info.present) {
        log::warn << "Property is not present: " << type << log::endl;
        return HeraErrno::ImmutableParameter;
    }

    log::debug << "Flir: Get property, type = " << type << ", absMin " << property_info.absMin << ", absMax "
               << property_info.absMax << log::endl;

    FlyCapture2::Property property;
    property.type = type;
    property.autoManualMode = (auto_set && property_info.autoSupported);
    property.absControl = property_info.absValSupported;
    property.onOff = property_info.onOffSupported;

    log::debug << "Flir: Set property, type = " << type << ", value = " << value << log::endl;

    if (value < property_info.absMin) {
        value = property_info.absMin;
    } else if (value > property_info.absMax) {
        value = property_info.absMax;
    }
    property.absValue = value;

    return handle_flir_error(camera_.SetProperty(&property));
}

HeraErrno DevicePlugin::set_range_auto_exposure()
{
    constexpr auto propRegister = 0x1088;
    uint32_t propRegVal = 0;

    const uint16_t min_value_int = local_parameters_.get_MinExposure() / ExposureGranularity;
    const uint16_t max_value_int = local_parameters_.get_MaxExposure() / ExposureGranularity;
    propRegVal += min_value_int << 12;
    propRegVal += max_value_int;

    return handle_flir_error(camera_.WriteRegister(propRegister, propRegVal));
}

HeraErrno DevicePlugin::set_range_auto_shutter()
{
    constexpr auto propRegister = 0x1098;
    uint32_t propRegVal = 0;

    auto ShutterGranularity = ShutterTimeGranularity23S6CMode7;
    if (local_parameters_.get_Binning() == +Binning::B22) {
        ShutterGranularity = ShutterTimeGranularity23S6CMode4;
    }
    const uint16_t min_value_int = local_parameters_.get_MinShutter() / ShutterGranularity;
    const uint16_t max_value_int = local_parameters_.get_MaxShutter() / ShutterGranularity;
    propRegVal += min_value_int << 12;
    propRegVal += max_value_int;

    return handle_flir_error(camera_.WriteRegister(propRegister, propRegVal));
}

HeraErrno DevicePlugin::set_range_auto_gain()
{
    constexpr auto propRegister = 0x10A0;
    uint32_t propRegVal = 0;

    const uint16_t min_value_int = local_parameters_.get_MinShutter() / GainGranularity;
    const uint16_t max_value_int = local_parameters_.get_MaxShutter() / GainGranularity;
    propRegVal += min_value_int << 12;
    propRegVal += max_value_int;

    return handle_flir_error(camera_.WriteRegister(propRegister, propRegVal));
}

bool DevicePlugin::test_nvm_availability()
{
    uint32_t regVal;
    if (camera_.ReadRegister(NVMControlRegister, &regVal) != FlyCapture2::PGRERROR_OK) {
        return false;
    }

    return (regVal & 0x80000000) == 0 ? false : true;
}

bool DevicePlugin::commit_nvm()
{
    uint32_t regVal;
    if (camera_.ReadRegister(NVMControlRegister, &regVal) != FlyCapture2::PGRERROR_OK) {
        return false;
    }

    regVal |= 1 << (31 - 6);
    return camera_.WriteRegister(NVMControlRegister, regVal) == FlyCapture2::PGRERROR_OK;
}

HeraErrno DevicePlugin::save_cc_parameter(const driver::ColorCorrectionParameter& cc_param)
{
    uint32_t flashOffset;
    constexpr size_t sizeCCQuad = sizeof(cc_param) / 4;

    if (!test_nvm_availability()) {
        return handle_error(HeraErrno::CanNotOpenCamera,
                            std::string("Non volatile memory not available on this camera"));
    }

    auto error = camera_.ReadRegister(NVMOffsetRegister, &flashOffset);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_error(HeraErrno::CanNotOpenCamera, std::string("Read Flash Offset, ") + error.GetDescription());
    }

    const uint32_t* src = (const uint32_t*)&cc_param;
    uint32_t src_checked[sizeCCQuad + 2];
    src_checked[sizeCCQuad] = 0;
    src_checked[sizeCCQuad + 1] = CCMagic;
    for (size_t i = 0; i < sizeCCQuad; ++i) {
        src_checked[i] = src[i];
        src_checked[sizeCCQuad] ^= src[i];
    }

    log::debug << "Flir: Flash Offset = " << flashOffset << log::endl;

    log::debug << "Flir: Saveing Parameter into Flash" << log::endl;

    error = camera_.WriteRegisterBlock(0xFFFF, 0xF0000000 + (flashOffset * 4), src_checked, sizeCCQuad + 2);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_error(HeraErrno::CanNotOpenCamera,
                            std::string("Write CC Parameter in Flash Block, ") + error.GetDescription());
    }

    if (!commit_nvm()) {
        return handle_error(HeraErrno::CanNotOpenCamera, "Failed to commit data into nvm");
    }

    driver::ColorCorrectionParameter cc_param_read;
    auto h_error = load_cc_parameter(cc_param_read);
    if (h_error != HeraErrno::OK) {
        return h_error;
    }

    log::debug << "Flir: Comparing Parameter from Flash" << log::endl;

    if (memcmp(&cc_param_read, &cc_param, sizeof(cc_param))) {
        return handle_error(HeraErrno::CanNotOpenCamera, "Parameter Loaded from Flash differs from Written");
    }

    return HeraErrno::OK;
}

HeraErrno DevicePlugin::load_cc_parameter(driver::ColorCorrectionParameter& cc_param)
{
    uint32_t flashOffset;
    constexpr size_t sizeCCQuad = sizeof(cc_param) / 4;

    if (!test_nvm_availability()) {
        return handle_error(HeraErrno::CanNotOpenCamera,
                            std::string("Non volatile memory not available on this camera"));
    }

    log::debug << "Flir: Loading Parameter from Flash" << log::endl;

    auto error = camera_.ReadRegister(NVMOffsetRegister, &flashOffset);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_error(HeraErrno::CanNotOpenCamera, std::string("Read Flash Offset, ") + error.GetDescription());
    }

    uint32_t read_data[sizeCCQuad + 2];

    error = camera_.ReadRegisterBlock(0xFFFF, 0xF0000000 + (flashOffset * 4), read_data, sizeCCQuad + 2);
    if (error != FlyCapture2::PGRERROR_OK) {
        return handle_error(HeraErrno::CanNotOpenCamera,
                            std::string("Read CC Parameter in Flash Block, ") + error.GetDescription());
    }

    uint32_t checksum = 0;
    for (size_t i = 0; i < sizeCCQuad + 1; ++i) {
        checksum ^= read_data[i];
    }
    if (checksum != 0) {
        return handle_error(HeraErrno::CanNotOpenCamera,
                            "Can not load CC Parameter from Flash, since checksum mismatched");
    }
    if (CCMagic != read_data[sizeCCQuad + 1]) {
        return handle_error(HeraErrno::CanNotOpenCamera,
                            "Can not load CC Parameter from Flash, since magic mismatched, data read is" +
                                    std::to_string(read_data[sizeCCQuad + 1]));
    }

    memcpy(&cc_param, read_data, sizeof(cc_param));

    return HeraErrno::OK;
}

HeraErrno DevicePlugin::handle_flir_error(const FlyCapture2::Error& error, const std::string& extra)
{
    if (error == FlyCapture2::PGRERROR_OK) {
        return HeraErrno::OK;
    } else {
        return handle_error(HeraErrno::CanNotOpenCamera, extra + error.GetDescription());
    }
}
#endif

/// For jpeg format, just copy
///
data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameters)
{
    if (storage_data->is_type(DeviceDataType::CameraFlirCompressedImage)) {
        // Raw DeviceData of Derived Type
        auto raw_data = static_cast<FlirCompressedImage*>(storage_data.get());

        // Create a SensorData from DeviceData
        uint32_t image_data_size = raw_data->image_data_size;
        uint32_t length = sizeof(data::CompressedImage) + image_data_size;
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::CompressedImage, length);
        auto camera_sensor_data = static_cast<data::CompressedImage*>(sensor_data.get());

        // Parse Data
        camera_sensor_data->timestamp_intrinsic_ns = raw_data->timestamp_intrinsic_ns;
        camera_sensor_data->compress_format = raw_data->compress_format;
        camera_sensor_data->image_data_size = image_data_size;
        memcpy(camera_sensor_data->image_data, &(raw_data->image_data), image_data_size);
        return sensor_data;
    } else if (storage_data->is_type(DeviceDataType::CameraFlirRawImage)) {
        // Raw DeviceData of Derived Type
        auto raw_data = static_cast<FlirRawImage*>(storage_data.get());

        // Create a SensorData from DeviceData
        uint32_t image_data_size = raw_data->image_data_size;
        uint32_t length = sizeof(data::Image) + image_data_size;
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::Image, length);
        auto camera_sensor_data = static_cast<data::Image*>(sensor_data.get());

        // Parse Data
        camera_sensor_data->timestamp_intrinsic_ns = raw_data->timestamp_intrinsic_ns;
        camera_sensor_data->image_meta = raw_data->image_meta;
        camera_sensor_data->image_data_size = image_data_size;
        memcpy(camera_sensor_data->image_data, &(raw_data->image_data), image_data_size);
        return sensor_data;
    }
    return data::SensorData::broken_data();
}

}  // namespace flir
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz