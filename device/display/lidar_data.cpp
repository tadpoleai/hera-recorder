///
/// @file lidar_data.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Implementation of Display::parse for SensorData for Lidar
/// @date 2019-12-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "data/lidar_data.hpp"

#include <algorithm>
#include <cstring>
#include <turbojpeg.h>

#include "common/include/logger/logger.hpp"
#include "display_data.hpp"
#include "sensor_data_types.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

class ColorProfileType {
public:
    constexpr ColorProfileType() : data()
    {
        for (auto i = 0; i != LUTSize; ++i) {
            double hue = 2.0 * double(i) / double(MaxIntensity);
            if (hue < 1.0) {
                data[3 * i + 0] = std::min(255.0, (255 - Saturation) + Saturation * (1.0 - hue));
                data[3 * i + 1] = std::min(255.0, (255 - Saturation) + Saturation * (0.0 + hue));
                data[3 * i + 2] = std::min(255.0, (255 - Saturation));
            } else {
                data[3 * i + 0] = std::min(255.0, (255 - Saturation));
                data[3 * i + 1] = std::min(255.0, (255 - Saturation) + Saturation * (2.0 - hue));
                data[3 * i + 2] = std::min(255.0, (255 - Saturation) + Saturation * (hue - 1.0));
            }
        }
    }

    inline void fill_pixel(uint8_t* pixel, float intensity) const
    {
        uint8_t intensity_uint8 = static_cast<uint8_t>(intensity);
        if (intensity_uint8 > MaxIntensity) {
            intensity_uint8 = MaxIntensity;
        }
        memcpy(pixel, &data[3 * intensity_uint8], 3);
    }

    inline void fill_pixel_gray(uint8_t* pixel, float intensity) const
    {
        uint8_t intensity_uint8 = static_cast<uint8_t>(intensity);
        if (intensity_uint8 > MaxIntensity) {
            intensity_uint8 = MaxIntensity;
        }
        intensity_uint8 += GrayIntensityShift;
        *pixel++ = intensity_uint8;
        *pixel++ = intensity_uint8;
        *pixel++ = intensity_uint8;
    }

private:
    static constexpr double Saturation = 180;
    static constexpr auto MaxIntensity = 80;
    static constexpr auto GrayIntensityShift = 128;
    static constexpr auto LUTSize = MaxIntensity + 1;
    uint8_t data[3 * LUTSize];
};

template<>
SingleDisplayData SingleDisplayData::parse<SensorDataType::Points>(std::vector<SensorDataPtr>&& sensor_datas,
                                                                   const bool is_detail)
{
    static constexpr auto CanvasPixelSize = 600;
    static constexpr auto CanvasPixelCenter = CanvasPixelSize / 2;
    static constexpr auto CanvasPixelNum = CanvasPixelSize * CanvasPixelSize;
    static constexpr auto CanvasDataSize = 3 * CanvasPixelNum;
    static constexpr float GranularityMeter = 0.03;
    static constexpr auto JpegQuality = 95;
    static constexpr auto BgBrightness = 0;
    static constexpr ColorProfileType ColorProfile;

    auto canvas = std::unique_ptr<uint8_t[]>(new uint8_t[CanvasDataSize]);
    auto render_history = std::unique_ptr<float[]>(new float[CanvasPixelNum]);
    memset(canvas.get(), BgBrightness, CanvasDataSize);
    for (auto&& data : sensor_datas) {
        if (data->sensor_data_type != SensorDataType::Points) {
            continue;
        }

        auto data_impl = reinterpret_cast<data::Points*>(data.get());
        bool is_dual = (data_impl->meta.return_type == data::Points::ReturnType::Dual);
        if (!is_dual) {
            for (size_t i = 0; i < data_impl->point_number; ++i) {
                auto* pt = &data_impl->points[i];
                int pixel_x = CanvasPixelCenter - pt->y / GranularityMeter;
                int pixel_y = CanvasPixelCenter - pt->x / GranularityMeter;
                if (pixel_x >= 0 && pixel_x < CanvasPixelSize && pixel_y >= 0 && pixel_y < CanvasPixelSize) {
                    auto offset = pixel_y * CanvasPixelSize + pixel_x;
                    ColorProfile.fill_pixel(&canvas[3 * offset], pt->intensity);
                }
            }
        } else {
            for (size_t i = 0; i < data_impl->point_number / 2; ++i) {
                auto* pt = &data_impl->points[i];
                auto* pt_2 = &data_impl->points[i + data_impl->point_number / 2];

                int pixel_x = CanvasPixelCenter - pt->y / GranularityMeter;
                int pixel_y = CanvasPixelCenter - pt->x / GranularityMeter;
                if (pixel_x >= 0 && pixel_x < CanvasPixelSize && pixel_y >= 0 && pixel_y < CanvasPixelSize) {
                    auto offset = pixel_y * CanvasPixelSize + pixel_x;
                    ColorProfile.fill_pixel(&canvas[3 * offset], pt->intensity);
                }

                if (pt->horizontal_distance - pt_2->horizontal_distance > 1e-5) {
                    int pixel_x = CanvasPixelCenter - pt_2->y / GranularityMeter;
                    int pixel_y = CanvasPixelCenter - pt_2->x / GranularityMeter;
                    if (pixel_x >= 0 && pixel_x < CanvasPixelSize && pixel_y >= 0 && pixel_y < CanvasPixelSize) {
                        auto offset = pixel_y * CanvasPixelSize + pixel_x;
                        ColorProfile.fill_pixel_gray(&canvas[3 * offset], pt_2->intensity);
                    }
                }
            }
        }
    }

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
    return result;
}

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
