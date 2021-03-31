///
/// @file velodyne_defs.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Constant definitions of Velodyne's lidars
/// @version 0.1
/// @date 2019-11-07
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "common/include/utils/time.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace lidar {
namespace hesai {

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
/// @brief Granularity of Velodyne's lidars azimuth
///
/// From rotation per minute to rad per second
static constexpr double MotorSpeedGranularity = 2 * M_PI / 60.0;

///
/// @brief Number of DataBlocks in every Hesai UDP Packet
///
static constexpr auto NumDataBlockPerPacket = 8;

///
/// @brief Number of Channels in every DataBlock
///
static constexpr auto NumChannelPerDataBlock = 32;

///
/// @brief Number of Points in every Velodyne UDP Packet
///
static constexpr auto NumPointsPerPacket = NumChannelPerDataBlock * NumDataBlockPerPacket;

///
/// @brief Namespace for PandarXT
///
namespace pandarxt32 {

static constexpr double MinNominalRange = 0.0;
static constexpr double MaxNominalRange = 120;

static constexpr double TimePerPoint = 1512;      ///< [ns]
static constexpr double TimeHorizontal = 50000;   ///< per block [ns]
static constexpr double TimeOffset = 3280 + 280;  ///< [ns]

static constexpr double DistanceGranularity = 0.004;  ///< 4mm

///
/// @brief Get the Azimuth Change
///
/// @param channel lidar channel, ranges [0, 32)
///
/// @return time change in [sec]
static constexpr double GetAbsoluteTimeChange(size_t channel) noexcept
{
    return TimePerPoint * channel / (double)time::OneSecond;
}

///
/// @brief Vertical angle (aka pitch) of every channel, in rad
///
static constexpr double VerticalAngles[32] = {+15.0 * DegreeToRad, +14.0 * DegreeToRad, +13.0 * DegreeToRad,
                                              +12.0 * DegreeToRad, +11.0 * DegreeToRad, +10.0 * DegreeToRad,
                                              +09.0 * DegreeToRad, +08.0 * DegreeToRad, +07.0 * DegreeToRad,
                                              +06.0 * DegreeToRad, +05.0 * DegreeToRad, +04.0 * DegreeToRad,
                                              +03.0 * DegreeToRad, +02.0 * DegreeToRad, +01.0 * DegreeToRad,
                                              +00.0 * DegreeToRad, -01.0 * DegreeToRad, -02.0 * DegreeToRad,
                                              -03.0 * DegreeToRad, -04.0 * DegreeToRad, -05.0 * DegreeToRad,
                                              -06.0 * DegreeToRad, -07.0 * DegreeToRad, -08.0 * DegreeToRad,
                                              -09.0 * DegreeToRad, -10.0 * DegreeToRad, -11.0 * DegreeToRad,
                                              -12.0 * DegreeToRad, -13.0 * DegreeToRad, -14.0 * DegreeToRad,
                                              -15.0 * DegreeToRad, -16.0 * DegreeToRad};

static constexpr double VerticalAngleIncrement = +01.000000000000000 * DegreeToRad;

}  // namespace pandarxt32

}  // namespace hesai
}  // namespace lidar
}  // namespace device
}  // namespace hera
}  // namespace wayz