///
/// @file velodyne_defs.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Constant definitions of Velodyne's lidars
/// @version 0.1
/// @date 2019-11-07
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <cmath>
#include <cstdint>
#include <string>

namespace wayz {
namespace hera {
namespace device {
namespace lidar {
namespace velodyne {

///
/// @brief Multiplier from rad to degree
///
static constexpr double DegreeToRad = M_PI / 180.0;

///
/// @brief Granularity of Velodyne's lidars azimuth
///
/// From hundredth of degree to rad
static constexpr double AzimuthGranularity = M_PI / 18000.0;

///
/// @brief Namespace for Velodyne VLP-16C
///
/// @anchor VLP-16C-Manual
/// @see <a href="https://shorturl.at/bnsxY" target="_blank"
/// rel="noopener noreferrer">VLP-16C-User-Manual</a>
namespace vlp16c {

///
/// @brief Granularity of Velodyne VLP-16C's distance
///
/// @see @ref VLP-16C-Manual section: 9.3.1.3, Data Point, Page 55
static constexpr double DistanceGranularity = 0.002;

static constexpr double MinNominalRange = 0.4;
static constexpr double MaxNominalRange = 100;

static constexpr double TimePerPoint = 2304;
static constexpr double TimeHorizontal = 55296;

///
/// @brief Get the Azimuth Change
///
/// @param index channel, ranges [0, 15]
/// @return constexpr double, relative azimuth change, ranges [0, 1)
/// @see @ref VLP-16C-Manual figure: 9-7, Single Return Mode Timing Offsets (in μs), page: 64
static constexpr double GetRelativeAzimuthChange(size_t index) noexcept
{
    if (index < 16) {
        return TimePerPoint / (2 * TimeHorizontal) * index;
    } else {
        return 0.5 + TimePerPoint / (2 * TimeHorizontal) * index;
    }
}

///
/// @brief Vertical angle (aka pitch) of every channel, in rad
///
/// @see @ref VLP-16C-Manual table: Vertical Angles by Laser ID and Model, page: 53 - 54
static constexpr double VerticalAngles[16] = {
        -15.000000000000000 * DegreeToRad,  // 0
        +01.000000000000000 * DegreeToRad,  // 1
        -13.000000000000000 * DegreeToRad,  // 2
        +03.000000000000000 * DegreeToRad,  // 3
        -11.000000000000000 * DegreeToRad,  // 4
        +05.000000000000000 * DegreeToRad,  // 5
        -09.000000000000000 * DegreeToRad,  // 6
        +07.000000000000000 * DegreeToRad,  // 7
        -07.000000000000000 * DegreeToRad,  // 8
        +09.000000000000000 * DegreeToRad,  // 9
        -05.000000000000000 * DegreeToRad,  // 10
        +11.000000000000000 * DegreeToRad,  // 11
        -03.000000000000000 * DegreeToRad,  // 12
        +13.000000000000000 * DegreeToRad,  // 13
        -01.000000000000000 * DegreeToRad,  // 14
        +15.000000000000000 * DegreeToRad,  // 15
};

static constexpr double VerticalAngleIncrement = +02.000000000000000 * DegreeToRad;

///
/// @brief Vertical correction (aka height) of every channel, in meter
///
/// @see @ref VLP-16C-Manual table: Vertical Angles by Laser ID and Model, page: 53 - 54
static constexpr double VerticalCorrection[16] = {
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
}  // namespace vlp16c

///
/// @brief Namespace for Velodyne VLP-32C
///
/// @anchor VLP-32C-Manual
/// @see <a href="https://shorturl.at/ksLMP" target="_blank"
/// rel="noopener noreferrer">VLP-32C-User-Manual</a>
namespace vlp32c {

///
/// @brief Granularity of Velodyne VLP-32C's distance
///
/// @see @ref VLP-32C-Manual section: 9.3.1.3, Data Point, Page 54
static constexpr double DistanceGranularity = 0.004;

static constexpr double MinNominalRange = 0.4;
static constexpr double MaxNominalRange = 200;

static constexpr double TimePerPoint = 2304;
static constexpr double TimeHorizontal = 55296;

///
/// @brief Get the Azimuth Change
///
/// @param index channel, ranges [0, 31]
/// @return constexpr double, relative azimuth change, ranges [0, 1)
/// @see @ref VLP-32C-Manual figure: 9-7, Single Return Mode Timing Offsets (in μs), page: 64
static constexpr double GetRelativeAzimuthChange(size_t index) noexcept
{
    return TimePerPoint / TimeHorizontal * (index / 2);
}

///
/// @brief Vertical angle (aka pitch) of every channel, in rad
///
/// @see @ref VLP-32C-Manual table: VLP-32C Data Order in Data Block, page: 57 - 58
static constexpr double VerticalAngles[32] = {
        -25.000000000000000 * DegreeToRad,  // 0
        -01.000000000000000 * DegreeToRad,  // 1
        -01.666666666666666 * DegreeToRad,  // 2
        -15.639000000000000 * DegreeToRad,  // 3
        -11.310000000000000 * DegreeToRad,  // 4
        +00.000000000000000 * DegreeToRad,  // 5
        -00.666666666666666 * DegreeToRad,  // 6
        -08.843000000000000 * DegreeToRad,  // 7
        -07.254000000000000 * DegreeToRad,  // 8
        +00.333333333333333 * DegreeToRad,  // 9
        -00.333333333333333 * DegreeToRad,  // 10
        -06.148000000000000 * DegreeToRad,  // 11
        -05.333333333333333 * DegreeToRad,  // 12
        +01.333333333333333 * DegreeToRad,  // 13
        +00.666666666666666 * DegreeToRad,  // 14
        -04.000000000000000 * DegreeToRad,  // 15
        -04.666666666666666 * DegreeToRad,  // 16
        +01.666666666666666 * DegreeToRad,  // 17
        +01.000000000000000 * DegreeToRad,  // 18
        -03.666666666666666 * DegreeToRad,  // 19
        -03.333333333333333 * DegreeToRad,  // 20
        +03.333333333333333 * DegreeToRad,  // 21
        +02.333333333333333 * DegreeToRad,  // 22
        -02.666666666666666 * DegreeToRad,  // 23
        -03.000000000000000 * DegreeToRad,  // 24
        +07.000000000000000 * DegreeToRad,  // 25
        +04.666666666666666 * DegreeToRad,  // 26
        -02.333333333333333 * DegreeToRad,  // 27
        -02.000000000000000 * DegreeToRad,  // 28
        +15.000000000000000 * DegreeToRad,  // 29
        +10.333333333333333 * DegreeToRad,  // 30
        -01.333333333333333 * DegreeToRad,  // 31
};

static constexpr double VerticalAngleIncrement = 00.333333333333333 * DegreeToRad;

///
/// @brief Horizontal azimuth offset of every channel, in rad
///
/// @see @ref VLP-32C-Manual table: VLP-32C Data Order in Data Block, page: 57 - 58
static constexpr double AzimuthOffset[32] = {
        +01.400000000000000 * DegreeToRad,  // 0
        -04.200000000000000 * DegreeToRad,  // 1
        +01.400000000000000 * DegreeToRad,  // 2
        -01.400000000000000 * DegreeToRad,  // 3
        +01.400000000000000 * DegreeToRad,  // 4
        -01.400000000000000 * DegreeToRad,  // 5
        +04.200000000000000 * DegreeToRad,  // 6
        -01.400000000000000 * DegreeToRad,  // 7
        +01.400000000000000 * DegreeToRad,  // 8
        -04.200000000000000 * DegreeToRad,  // 9
        +01.400000000000000 * DegreeToRad,  // 10
        -01.400000000000000 * DegreeToRad,  // 11
        +04.200000000000000 * DegreeToRad,  // 12
        -01.400000000000000 * DegreeToRad,  // 13
        +04.200000000000000 * DegreeToRad,  // 14
        -01.400000000000000 * DegreeToRad,  // 15
        +01.400000000000000 * DegreeToRad,  // 16
        -04.200000000000000 * DegreeToRad,  // 17
        +01.400000000000000 * DegreeToRad,  // 18
        -04.200000000000000 * DegreeToRad,  // 19
        +04.200000000000000 * DegreeToRad,  // 20
        -01.400000000000000 * DegreeToRad,  // 21
        +01.400000000000000 * DegreeToRad,  // 22
        -01.400000000000000 * DegreeToRad,  // 23
        +01.400000000000000 * DegreeToRad,  // 24
        -01.400000000000000 * DegreeToRad,  // 25
        +01.400000000000000 * DegreeToRad,  // 26
        -04.200000000000000 * DegreeToRad,  // 27
        +04.200000000000000 * DegreeToRad,  // 28
        -01.400000000000000 * DegreeToRad,  // 29
        +01.400000000000000 * DegreeToRad,  // 30
        -01.400000000000000 * DegreeToRad,  // 31
};
}  // namespace vlp32c

///
/// @brief Namespace for Velodyne HDL-32E
///
/// @anchor HDL-32E-Manual
/// @see <a href="https://shorturl.at/ghGYZ" target="_blank"
/// rel="noopener noreferrer">HDL-32E-User-Manual</a>
namespace hdl32e {

///
/// @brief Granularity of Velodyne HDL-32E's distance
///
/// @see @ref HDL-32E-Manual section: 9.3.1.3, Data Point, Page 59
static constexpr double DistanceGranularity = 0.002;

static constexpr double MinNominalRange = 0.4;
static constexpr double MaxNominalRange = 100;

static constexpr double TimePerPoint = 1152;
static constexpr double TimeHorizontal = 46080;

///
/// @brief Get the Azimuth Change
///
/// @param index channel, ranges [0, 31]
/// @return constexpr double, relative azimuth change, ranges [0, 1)
/// @see @ref HDL-32E-Manual figure: 9-6, Single Return Mode Timing Offsets (in μs), page: 67
static constexpr double GetRelativeAzimuthChange(size_t index) noexcept
{
    return TimePerPoint / TimeHorizontal * index;
}

static constexpr double AzimuthCorrection = M_PI_2;

///
/// @brief Vertical angle (aka pitch) of every channel, in rad
///
/// @see @ref HDL-32E-Manual table: HDL-32E Laser Firing Order, page: 62 - 63
static constexpr double VerticalAngles[32] = {
        -30.666666666666666 * DegreeToRad,  // 1
        -09.333333333333333 * DegreeToRad,  // 2
        -29.333333333333333 * DegreeToRad,  // 3
        -08.000000000000000 * DegreeToRad,  // 4
        -28.000000000000000 * DegreeToRad,  // 5
        -06.666666666666666 * DegreeToRad,  // 6
        -26.666666666666666 * DegreeToRad,  // 7
        -05.333333333333333 * DegreeToRad,  // 8
        -25.333333333333333 * DegreeToRad,  // 9
        -04.000000000000000 * DegreeToRad,  // 10
        -24.000000000000000 * DegreeToRad,  // 11
        -02.666666666666666 * DegreeToRad,  // 12
        -22.666666666666666 * DegreeToRad,  // 13
        -01.333333333333333 * DegreeToRad,  // 14
        -21.333333333333333 * DegreeToRad,  // 15
        -00.000000000000000 * DegreeToRad,  // 16
        -20.000000000000000 * DegreeToRad,  // 17
        +01.333333333333333 * DegreeToRad,  // 18
        -18.666666666666666 * DegreeToRad,  // 19
        +02.666666666666666 * DegreeToRad,  // 20
        -17.333333333333333 * DegreeToRad,  // 21
        +04.000000000000000 * DegreeToRad,  // 22
        -16.000000000000000 * DegreeToRad,  // 23
        +05.333333333333333 * DegreeToRad,  // 24
        -14.666666666666666 * DegreeToRad,  // 25
        +06.666666666666666 * DegreeToRad,  // 26
        -13.333333333333333 * DegreeToRad,  // 27
        +08.000000000000000 * DegreeToRad,  // 28
        -12.000000000000000 * DegreeToRad,  // 29
        +09.333333333333333 * DegreeToRad,  // 30
        -10.666666666666666 * DegreeToRad,  // 31
        +10.666666666666666 * DegreeToRad,  // 32
};

static constexpr double VerticalAngleIncrement = 01.333333333333333 * DegreeToRad;

}  // namespace hdl32e

}  // namespace velodyne
}  // namespace lidar
}  // namespace device
}  // namespace hera
}  // namespace wayz