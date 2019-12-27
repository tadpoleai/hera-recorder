///
/// @file lidar_data.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Implementation of Display::parse for SensorData for Lidar
/// @date 2019-12-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "lidar_data.hpp"

#include <algorithm>
#include <turbojpeg.h>

#include "common/logger/logger.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

static inline void fill_pixel_color(uint8_t* pixel, float height, float intensity)
{
    static constexpr float HeightRangeMeter = 0.5f;

    intensity = 180.0 + 0.5 * intensity;
    float nor_height = std::max(-1.0f, std::min(1.0f, height / HeightRangeMeter));
    if (nor_height > 0) {
        pixel[0] = std::min(255.0f, nor_height * intensity);
        pixel[1] = std::min(255.0f, (1.0f - nor_height) * intensity);
    } else {
        pixel[1] = std::min(255.0f, (1.0f + nor_height) * intensity);
        pixel[2] = std::min(255.0f, -nor_height * intensity);
    }
}

template<>
std::string DisplayData::parse<SensorDataType::PointsXYZI>(std::vector<SensorDataPtr>&& sensor_datas, bool& is_jpeg)
{
    static constexpr auto NoData = "No data";
    static constexpr auto CanvasPixelSize = 320;
    static constexpr auto CanvasPixelCenter = CanvasPixelSize / 2;
    static constexpr auto CanvasPixelNum = CanvasPixelSize * CanvasPixelSize;
    static constexpr auto CanvasDataSize = 3 * CanvasPixelNum;
    static constexpr float GranularityMeter = 0.05;
    static constexpr auto JpegQuality = 90;

    is_jpeg = true;
    auto canvas = std::unique_ptr<uint8_t[]>(new uint8_t[CanvasDataSize]);
    auto render_history = std::unique_ptr<float[]>(new float[CanvasPixelNum]);
    memset(canvas.get(), 64, CanvasDataSize);
    for (auto&& data : sensor_datas) {
        if (data->sensor_data_type != SensorDataType::PointsXYZI) {
            continue;
        }

        auto data_impl = reinterpret_cast<data::PointsXYZI*>(data.get());
        for (size_t i = 0; i < data_impl->point_number; ++i) {
            auto* pt = &data_impl->points[i];
            int pixel_x = CanvasPixelCenter - pt->y / GranularityMeter;
            int pixel_y = CanvasPixelCenter - pt->x / GranularityMeter;
            if (pixel_x >= 0 && pixel_x < CanvasPixelSize && pixel_y >= 0 && pixel_y < CanvasPixelSize) {
                auto offset = pixel_y * CanvasPixelSize + pixel_x;
                fill_pixel_color(&canvas[3 * offset], pt->z, pt->intensity);
            }
        }
    }

    auto tj_instance = tjInitCompress();
    if (!tj_instance) {
        return NoData;
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
        return NoData;
    }

    is_jpeg = true;
    std::string result;
    result.resize(dst_size);
    memcpy((void*)result.data(), dst_image, dst_size);

    tjDestroy(tj_instance);
    tjFree(dst_image);
    return result;
}

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
