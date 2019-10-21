//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "./lidar.hpp"

namespace wayz {
namespace tron {

static constexpr double DegreeToRad_ = M_PI / 180.0;
// For more details, refer to the following document
// pdf: VLP-16 User Manual Note, table: Vertical Angles by Laser ID and Model, page: 53 - 54
double Lidar::VerticalAngles16C_[16] = {
        -15.000000000000000 * DegreeToRad_,  // 0
        +01.000000000000000 * DegreeToRad_,  // 1
        -13.000000000000000 * DegreeToRad_,  // 2
        +03.000000000000000 * DegreeToRad_,  // 3
        -11.000000000000000 * DegreeToRad_,  // 4
        +05.000000000000000 * DegreeToRad_,  // 5
        -09.000000000000000 * DegreeToRad_,  // 6
        +07.000000000000000 * DegreeToRad_,  // 7
        -07.000000000000000 * DegreeToRad_,  // 8
        +09.000000000000000 * DegreeToRad_,  // 9
        -05.000000000000000 * DegreeToRad_,  // 10
        +11.000000000000000 * DegreeToRad_,  // 11
        -03.000000000000000 * DegreeToRad_,  // 12
        +13.000000000000000 * DegreeToRad_,  // 13
        -01.000000000000000 * DegreeToRad_,  // 14
        +15.000000000000000 * DegreeToRad_,  // 15
};

double Lidar::VerticalCorrection16C_[16] = {
        +11.20000000000000 * 0.001,  // 0
        -00.70000000000000 * 0.001,  // 1
        +09.70000000000000 * 0.001,  // 2
        -02.20000000000000 * 0.001,  // 3
        +08.10000000000000 * 0.001,  // 4
        -03.70000000000000 * 0.001,  // 5
        +06.60000000000000 * 0.001,  // 6
        -05.10000000000000 * 0.001,  // 7
        +05.10000000000000 * 0.001,  // 8
        -06.60000000000000 * 0.001,  // 9
        +03.70000000000000 * 0.001,  // 10
        -08.10000000000000 * 0.001,  // 11
        +02.20000000000000 * 0.001,  // 12
        -09.70000000000000 * 0.001,  // 13
        +00.70000000000000 * 0.001,  // 14
        -11.20000000000000 * 0.001,  // 15
};

// For more details, refer to the following document
// pdf: VLP-32C User Manual, table: VLP-32C Data Order in Data Block, page: 57 - 58
double Lidar::VerticalAngles32C_[32] = {
        -25.000000000000000 * DegreeToRad_,  // 0
        -01.000000000000000 * DegreeToRad_,  // 1
        -01.666666666666666 * DegreeToRad_,  // 2
        -15.639000000000000 * DegreeToRad_,  // 3
        -11.310000000000000 * DegreeToRad_,  // 4
        +00.000000000000000 * DegreeToRad_,  // 5
        -00.666666666666666 * DegreeToRad_,  // 6
        -08.843000000000000 * DegreeToRad_,  // 7
        -07.254000000000000 * DegreeToRad_,  // 8
        +00.333333333333333 * DegreeToRad_,  // 9
        -00.333333333333333 * DegreeToRad_,  // 10
        -06.148000000000000 * DegreeToRad_,  // 11
        -05.333333333333333 * DegreeToRad_,  // 12
        +01.333333333333333 * DegreeToRad_,  // 13
        +00.666666666666666 * DegreeToRad_,  // 14
        -04.000000000000000 * DegreeToRad_,  // 15
        -04.666666666666666 * DegreeToRad_,  // 16
        +01.666666666666666 * DegreeToRad_,  // 17
        +01.000000000000000 * DegreeToRad_,  // 18
        -03.666666666666666 * DegreeToRad_,  // 19
        -03.333333333333333 * DegreeToRad_,  // 20
        +03.333333333333333 * DegreeToRad_,  // 21
        +02.333333333333333 * DegreeToRad_,  // 22
        -02.666666666666666 * DegreeToRad_,  // 23
        -03.000000000000000 * DegreeToRad_,  // 24
        +07.000000000000000 * DegreeToRad_,  // 25
        +04.666666666666666 * DegreeToRad_,  // 26
        -02.333333333333333 * DegreeToRad_,  // 27
        -02.000000000000000 * DegreeToRad_,  // 28
        +15.000000000000000 * DegreeToRad_,  // 29
        +10.333333333333333 * DegreeToRad_,  // 30
        -01.333333333333333 * DegreeToRad_,  // 31

};
double Lidar::AzimuthOffset32C_[32] = {
        +01.400000000000000 * DegreeToRad_,  // 0
        -04.200000000000000 * DegreeToRad_,  // 1
        +01.400000000000000 * DegreeToRad_,  // 2
        -01.400000000000000 * DegreeToRad_,  // 3
        +01.400000000000000 * DegreeToRad_,  // 4
        -01.400000000000000 * DegreeToRad_,  // 5
        +04.200000000000000 * DegreeToRad_,  // 6
        -01.400000000000000 * DegreeToRad_,  // 7
        +01.400000000000000 * DegreeToRad_,  // 8
        -04.200000000000000 * DegreeToRad_,  // 9
        +01.400000000000000 * DegreeToRad_,  // 10
        -01.400000000000000 * DegreeToRad_,  // 11
        +04.200000000000000 * DegreeToRad_,  // 12
        -01.400000000000000 * DegreeToRad_,  // 13
        +04.200000000000000 * DegreeToRad_,  // 14
        -01.400000000000000 * DegreeToRad_,  // 15
        +01.400000000000000 * DegreeToRad_,  // 16
        -04.200000000000000 * DegreeToRad_,  // 17
        +01.400000000000000 * DegreeToRad_,  // 18
        -04.200000000000000 * DegreeToRad_,  // 19
        +04.200000000000000 * DegreeToRad_,  // 20
        -01.400000000000000 * DegreeToRad_,  // 21
        +01.400000000000000 * DegreeToRad_,  // 22
        -01.400000000000000 * DegreeToRad_,  // 23
        +01.400000000000000 * DegreeToRad_,  // 24
        -01.400000000000000 * DegreeToRad_,  // 25
        +01.400000000000000 * DegreeToRad_,  // 26
        -04.200000000000000 * DegreeToRad_,  // 27
        +04.200000000000000 * DegreeToRad_,  // 28
        -01.400000000000000 * DegreeToRad_,  // 29
        +01.400000000000000 * DegreeToRad_,  // 30
        -01.400000000000000 * DegreeToRad_,  // 31
};

// For more details, refer to the following document
// pdf: HDL-32E User Manual table: HDL-32E Laser Firing Order, page: 62 - 63
double Lidar::VerticalAngles32E_[32] = {
        -30.666666666666666 * DegreeToRad_,  // 1
        -09.333333333333333 * DegreeToRad_,  // 2
        -29.333333333333333 * DegreeToRad_,  // 3
        -08.000000000000000 * DegreeToRad_,  // 4
        -28.000000000000000 * DegreeToRad_,  // 5
        -06.666666666666666 * DegreeToRad_,  // 6
        -26.666666666666666 * DegreeToRad_,  // 7
        -05.333333333333333 * DegreeToRad_,  // 8
        -25.333333333333333 * DegreeToRad_,  // 9
        -04.000000000000000 * DegreeToRad_,  // 10
        -24.000000000000000 * DegreeToRad_,  // 11
        -02.666666666666666 * DegreeToRad_,  // 12
        -22.666666666666666 * DegreeToRad_,  // 13
        -01.333333333333333 * DegreeToRad_,  // 14
        -21.333333333333333 * DegreeToRad_,  // 15
        -00.000000000000000 * DegreeToRad_,  // 16
        -20.000000000000000 * DegreeToRad_,  // 17
        +01.333333333333333 * DegreeToRad_,  // 18
        -18.666666666666666 * DegreeToRad_,  // 19
        +02.666666666666666 * DegreeToRad_,  // 20
        -17.333333333333333 * DegreeToRad_,  // 21
        +04.000000000000000 * DegreeToRad_,  // 22
        -16.000000000000000 * DegreeToRad_,  // 23
        +05.333333333333333 * DegreeToRad_,  // 24
        -14.666666666666666 * DegreeToRad_,  // 25
        +06.666666666666666 * DegreeToRad_,  // 26
        -13.333333333333333 * DegreeToRad_,  // 27
        +08.000000000000000 * DegreeToRad_,  // 28
        -12.000000000000000 * DegreeToRad_,  // 29
        +09.333333333333333 * DegreeToRad_,  // 30
        -10.666666666666666 * DegreeToRad_,  // 31
        +10.666666666666666 * DegreeToRad_,  // 32
};

}  // namespace tron
}  // namespace wayz