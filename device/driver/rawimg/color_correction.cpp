///
/// @file color_correction.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-26
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "color_correction.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "common/include/logger/logger.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace driver {

std::ostream& operator<<(std::ostream& os, const ColorCorrectionParameter& self)
{
    os << "Dark Level = (" << self.dark_level[0] << ", " << self.dark_level[1] << ", " << self.dark_level[2] << ")\n";
    os << "White Balance = (" << self.white_balance[0] << ", " << self.white_balance[1] << ", " << self.white_balance[2]
       << ")\n";
    os << "CCM = [" << self.color_correction_matrix[0] << ", " << self.color_correction_matrix[1] << ", "
       << self.color_correction_matrix[2] << "\n";
    os << "       " << self.color_correction_matrix[3] << ", " << self.color_correction_matrix[4] << ", "
       << self.color_correction_matrix[5] << "\n";
    os << "       " << self.color_correction_matrix[6] << ", " << self.color_correction_matrix[7] << ", "
       << self.color_correction_matrix[8] << "]\n";

    return os;
}

ColorCorrecterBGR16::ColorCorrecterBGR16(const ColorCorrectionParameter& param, const float gamma)
{
    for (uint32_t i = 0; i < (1 << 16); i++) {
        GammaLUT[i] = 0;
    }
    for (uint32_t i = (1 << 16); i < (2 << 16); i++) {
        GammaLUT[i] = ::pow((i - (1 << 16)) / (float)0xFFFF, 1.0 / gamma) * 0xFF;
    }
    for (uint32_t i = (2 << 16); i < (3 << 16); i++) {
        GammaLUT[i] = 0xFF;
    }

    for (auto ch = 0; ch < 3; ch++) {
        const uint16_t DarkLevelCh = param.dark_level[ch] * (1 << 16);
        const float DarkLevelRemainGain = 1 / (1 - param.dark_level[ch]);
        for (uint32_t level = 0; level < 0xFFFF; level++) {
            if (level < DarkLevelCh) {
                CCMLUT[ch][3 * level + 0] = 0;
                CCMLUT[ch][3 * level + 1] = 0;
                CCMLUT[ch][3 * level + 2] = 0;
            } else {
                CCMLUT[ch][3 * level + 0] =
                        param.color_correction_matrix[0 + ch] *
                        std::min((float)(1 << 16),
                                 DarkLevelRemainGain * param.white_balance[ch] * (level - DarkLevelCh));
                CCMLUT[ch][3 * level + 1] =
                        param.color_correction_matrix[3 + ch] *
                        std::min((float)(1 << 16),
                                 DarkLevelRemainGain * param.white_balance[ch] * (level - DarkLevelCh));
                CCMLUT[ch][3 * level + 2] =
                        param.color_correction_matrix[6 + ch] *
                        std::min((float)(1 << 16),
                                 DarkLevelRemainGain * param.white_balance[ch] * (level - DarkLevelCh));
            }

            if (ch == 0) {
                CCMLUT[ch][3 * level + 0] += (1 << 16);
                CCMLUT[ch][3 * level + 1] += (1 << 16);
                CCMLUT[ch][3 * level + 2] += (1 << 16);
            }
        }
    }
}

void ColorCorrecterBGR16::process(void* bgr_data, uint8_t* dest_data, uint32_t image_pixel_num)
{
    if (image_pixel_num % 4 != 0) {
        log::error << "Image CC: assert failed! image_pixel_num % 4 == 0" << log::endl;
        return;
    }

    PixelBGR16* src_iter = (PixelBGR16*)bgr_data;

    const PixelBGR16* SrcEnd = src_iter + image_pixel_num;

    while (src_iter < SrcEnd) {
        const int32_t* ccm_ptr_Bb = &CCMLUT[0][3 * src_iter->b];
        const int32_t* ccm_ptr_Bg = &CCMLUT[1][3 * src_iter->g];
        const int32_t* ccm_ptr_Br = &CCMLUT[2][3 * src_iter->r];
        const int32_t Bcc = *ccm_ptr_Bb++ + *ccm_ptr_Bg++ + *ccm_ptr_Br++;
        const int32_t Gcc = *ccm_ptr_Bb++ + *ccm_ptr_Bg++ + *ccm_ptr_Br++;
        const int32_t Rcc = *ccm_ptr_Bb + *ccm_ptr_Bg + *ccm_ptr_Br;

        *dest_data++ = GammaLUT[Bcc];
        *dest_data++ = GammaLUT[Gcc];
        *dest_data++ = GammaLUT[Rcc];
        ++src_iter;
    }
}

}  // namespace driver
}  // namespace device
}  // namespace hera
}  // namespace wayz