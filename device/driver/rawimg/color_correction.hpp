///
/// @file color_correction.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Raw Image Correction
/// @date 2020-08-25
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cstdint>
#include <iostream>

namespace wayz {
namespace hera {
namespace device {
namespace driver {

#pragma pack(push, 1)
class ColorCorrectionParameter {
    friend std::ostream& operator<<(std::ostream& os, const ColorCorrectionParameter& self);

public:
    ColorCorrectionParameter() :
        dark_level{0, 0, 0},
        white_balance{1, 1, 1},
        color_correction_matrix{1, 0, 0, 0, 1, 0, 0, 0, 1}
    {}

public:
    float dark_level[3];
    float white_balance[3];
    float color_correction_matrix[9];
};
#pragma pack(pop)

///
/// @brief Correct Color and Adjust Gamma of Color Image
///
/// Calculation is performed in 16bit fixed-point number
///
/// The equivalence formula in float image(uniformed to (0,1) for input image and output image) is below:
///
/// Output sBRG<3,1> = Gamma { CCM<3,3> @ Trunc[0,1]( WhiteBalanceDiag<3> * (U<3> - DarkLevelDiag<3>) * Input<3,1> ) }
///
/// where, Input<3,1> is raw sensor input, ranges [0, 1) (0, 65535 in 16bit fixed-point number)
/// 
class ColorCorrecterBGR16 final {
public:
    ///
    /// @brief Construct a new Color Correcter object
    ///
    /// @param param
    /// @param gamma
    ColorCorrecterBGR16(const ColorCorrectionParameter& param, const float gamma);

    void process(void* bgr_data, uint8_t* dest_data, uint32_t image_pixel_num);

private:
#pragma pack(push, 1)
    struct PixelBGR16 {
        uint16_t b;
        uint16_t g;
        uint16_t r;
    };
#pragma pack(pop)

private:
    uint8_t GammaLUT[3 << 16];
    int32_t CCMLUT[3][3 << 16];
};

}  // namespace driver
}  // namespace device
}  // namespace hera
}  // namespace wayz
