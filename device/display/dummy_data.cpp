///
/// @file dummy_data.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Implementation of Display::parse for SensorData for Dummy
/// @date 2019-12-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "data/dummy_data.hpp"

#include <cstring>
#include <turbojpeg.h>

#include "common/include/logger/logger.hpp"
#include "display_data.hpp"
#include "sensor_data_types.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

template<>
SingleDisplayData SingleDisplayData::parse<SensorDataType::Dummy>(std::vector<SensorDataPtr>&& sensor_datas)
{
    SingleDisplayData result;
    for (auto&& data : sensor_datas) {
        if (data->sensor_data_type == SensorDataType::Dummy) {
            log::debug << "Display: Parsing Dummy Data" << log::endl;
            auto data_impl = reinterpret_cast<data::Dummy*>(data.get());
            result.text_data += std::to_string(data_impl->sequence);
            result.text_data += " ";
            result.text_data += std::to_string(data_impl->int_value);
            result.text_data += "\n";
        }
    }
    return result;
}

template<>
SingleDisplayData SingleDisplayData::parse<SensorDataType::DummyImage>(std::vector<SensorDataPtr>&& sensor_datas)
{
    static constexpr auto CanvasPixelSize = 480;
    static constexpr auto CanvasPixelNum = CanvasPixelSize * CanvasPixelSize;
    static constexpr auto CanvasDataSize = 3 * CanvasPixelNum;
    static constexpr auto JpegQuality = 90;

    auto canvas = std::unique_ptr<uint8_t[]>(new uint8_t[CanvasDataSize]);
    auto render_history = std::unique_ptr<float[]>(new float[CanvasPixelNum]);
    memset(canvas.get(), 16 * (sensor_datas[0]->sequence % 16), CanvasDataSize);

    auto tj_instance = tjInitCompress();
    if (!tj_instance) {
        return {};
    }
    uint8_t* dst_image = nullptr;
    size_t dst_size = 0;

    if (tjCompress2(tj_instance,
                    canvas.get(),
                    CanvasPixelSize,
                    0,
                    CanvasPixelSize,
                    TJPF_BGR,     // Source Image Format
                    &dst_image,   // Output Image Pointer
                    &dst_size,    // Output Image Size
                    TJSAMP_420,   // YUV Binning
                    JpegQuality,  // Quality
                    TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) != 0) {
        log::warn << "Display: Can not compress jpeg" << log::endl;
        tjDestroy(tj_instance);
        if (dst_image) {
            tjFree(dst_image);
        }
        return {};
    }

    SingleDisplayData result;
    result.jpeg_data.resize(dst_size);
    memcpy((void*)result.jpeg_data.data(), dst_image, dst_size);

    tjDestroy(tj_instance);
    tjFree(dst_image);

    result.text_data = "ImageSize: " + std::to_string(CanvasPixelSize) + "x" + std::to_string(CanvasPixelSize);
    return result;
}
}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
