///
/// @file camera_data.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Implementation of Display::parse for SensorData for Camera
/// @date 2019-12-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "data/camera_data.hpp"

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
SingleDisplayData SingleDisplayData::parse<SensorDataType::CompressedImage>(std::vector<SensorDataPtr>&& sensor_datas)
{
    static constexpr auto MaxTargetWidth = 540;
    static constexpr auto TargetQuality = 60;

    auto tj_instance = tjInitDecompress();
    if (!tj_instance) {
        log::warn << "Display: Can not decompress jpeg" << log::endl;
        return {};
    }

    int src_width, src_height, src_subsamp, src_colorspace;
    auto data_impl = reinterpret_cast<data::CompressedImage*>(sensor_datas[0].get());
    if (tjDecompressHeader3(tj_instance,
                            data_impl->image_data,
                            data_impl->image_data_size,
                            &src_width,
                            &src_height,
                            &src_subsamp,
                            &src_colorspace) != 0) {
        log::warn << "Display: Can not decompress jpeg" << log::endl;
        tjDestroy(tj_instance);
        return {};
    }

    tjscalingfactor scalingfactor;
    scalingfactor.num = 1;
    if (src_width > 2 * MaxTargetWidth) {
        scalingfactor.denom = 4;
    } else if (src_width > MaxTargetWidth) {
        scalingfactor.denom = 2;
    } else {
        scalingfactor.denom = 1;
    }
    auto dst_width = TJSCALED(src_width, scalingfactor);
    auto dst_height = TJSCALED(src_height, scalingfactor);

    auto dst_rgb_size = dst_width * dst_height * 3;
    auto rgb_image = new uint8_t[dst_rgb_size];
    if (tjDecompress2(tj_instance,
                      data_impl->image_data,
                      data_impl->image_data_size,
                      rgb_image,
                      dst_width,
                      0,
                      dst_height,
                      TJPF_BGR,
                      TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) != 0) {
        log::warn << "Display: Can not decompress jpeg" << log::endl;
        tjDestroy(tj_instance);
        delete[] rgb_image;
        return {};
    }
    tjDestroy(tj_instance);

    tj_instance = tjInitCompress();
    if (!tj_instance) {
        return {};
    }
    uint8_t* dst_image = nullptr;
    size_t dst_size = 0;

    if (tjCompress2(tj_instance,
                    rgb_image,
                    dst_width,
                    0,
                    dst_height,
                    TJPF_RGB,       // Source Image Format
                    &dst_image,     // Output Image Pointer
                    &dst_size,      // Output Image Size
                    TJSAMP_420,     // YUV Binning
                    TargetQuality,  // Quality
                    TJFLAG_FASTDCT | TJFLAG_FASTUPSAMPLE) != 0) {
        log::warn << "Display: Can not re-compress jpeg" << log::endl;
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
