///
/// @file feedback.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Feed LocalizationResult back
/// @date 2020-04-20
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "feedback.hpp"

#include <cmath>

namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace feedback {

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VCAN

inline double clamp(const double src, const double min, const double max)
{
    if (src < min) {
        return min;
    } else if (src > max) {
        return max;
    }
    return src;
}

void feedback(const data::LocalizationResult* result, driver::CANPort* port)
{
    if (!port || !result) {
        return;
    }

    {
        // localizer status -- 7D1
        auto packet = driver::CANPacket::create(0x7d1, 8);
        packet->data[0] = (uint8_t)result->system_status & 0x0fu;
        packet->data[1] = (uint8_t)result->result_type & 0x0fu;
        port->write(packet);
    }

    {
        // orientation -- 7D2
        auto packet = driver::CANPacket::create(0x7d2, 8);
        constexpr double kOrientationFactor = 0.006;
        constexpr double kRadToDeg = 180. / M_PI;
        constexpr double kOrientationOffset = M_PI;
        const uint16_t heading = (result->orientation[0] + kOrientationOffset) * kRadToDeg / kOrientationFactor;
        const uint16_t pitch = (result->orientation[1] + kOrientationOffset) * kRadToDeg / kOrientationFactor;
        const uint16_t roll = (result->orientation[2] + kOrientationOffset) * kRadToDeg / kOrientationFactor;

        packet->data[0] = (heading >> 8);
        packet->data[1] = (heading & 0xff);
        packet->data[2] = (pitch >> 8);
        packet->data[3] = (pitch & 0xff);
        packet->data[4] = (roll >> 8);
        packet->data[5] = (roll & 0xff);
        port->write(packet);
    }

    // {
    //     // orientation variance -- 7D3
    //     auto packet = driver::CANPacket::create(0x7d3, 8);
    //     // todo
    // }

    // {
    //     // orientation covariance -- 7D4
    //     auto packet = driver::CANPacket::create(0x7d4, 8);
    //     // todo
    // }

    constexpr double kPositionDegreeFactor = 0.000000001;
    constexpr double kPositionMeterFactor = 0.001;
    constexpr double kLatitudeOffset = 90.;
    constexpr double kLongitudeOffset = 180.;
    {
        // position1 -- 7D5
        auto packet = driver::CANPacket::create(0x7d5, 8);
        double offseted_position_l = result->position[4] + kLongitudeOffset;
        offseted_position_l = clamp(offseted_position_l, 0., 360.);

        const uint64_t position_l = offseted_position_l / kPositionDegreeFactor;
        packet->data[3] = ((uint64_t)(position_l & 0xff00000000) >> 32);
        packet->data[4] = ((uint64_t)(position_l & 0xff000000) >> 24);
        packet->data[5] = ((uint64_t)(position_l & 0xff0000) >> 16);
        packet->data[6] = ((uint64_t)(position_l & 0xff00) >> 8);
        packet->data[7] = (position_l & 0xff);
        port->write(packet);
    }

    constexpr double kPositionENUOffset = 2097.;
    {
        // position2 -- 7d6
        auto packet = driver::CANPacket::create(0x7d6, 8);
        double offseted_pose_h = result->position[2] + kPositionENUOffset;
        if (offseted_pose_h < 0.) {
            offseted_pose_h = 1.e-6;
        }
        uint32_t position_h = offseted_pose_h / kPositionMeterFactor;
        position_h <<= 2;
        packet->data[0] = ((position_h & 0xff0000) >> 16);
        packet->data[1] = ((position_h & 0xff00) >> 8);
        packet->data[2] = (position_h & 0xff);

        double offseted_position_b = result->position[3] + kLatitudeOffset;
        offseted_position_b = clamp(offseted_position_b, 0., 180.);

        const uint64_t position_b = offseted_position_b / kPositionDegreeFactor;
        packet->data[3] = ((uint64_t)(position_b & 0xff00000000) >> 32);
        packet->data[4] = ((uint64_t)(position_b & 0xff000000) >> 24);
        packet->data[5] = ((uint64_t)(position_b & 0xff0000) >> 16);
        packet->data[6] = ((uint64_t)(position_b & 0xff00) >> 8);
        packet->data[7] = (position_b & 0xff);
        port->write(packet);
    }

    {
        // map anchor -- 7d7
        // lat
        auto packet = driver::CANPacket::create(0x7d7, 8);
        double offseted_lat = result->position_anchor[0] + kLatitudeOffset;
        offseted_lat = clamp(offseted_lat, 0., 180.);
        const uint64_t anchor_lat = offseted_lat / kPositionDegreeFactor;
        packet->data[0] = ((uint64_t)(anchor_lat & 0xff00000000) >> 32);
        packet->data[1] = ((uint64_t)(anchor_lat & 0xff000000) >> 24);
        packet->data[2] = ((uint64_t)(anchor_lat & 0xff0000) >> 16);
        packet->data[3] = ((uint64_t)(anchor_lat & 0xff00) >> 8);
        packet->data[4] = (anchor_lat & 0xff);
        // north
        double offseted_pose_n = result->position[1] + kPositionENUOffset;
        if (offseted_pose_n < 0.) {
            offseted_pose_n = 1.e-6;
        }
        uint32_t position_n = offseted_pose_n / kPositionMeterFactor;
        position_n <<= 2;
        packet->data[5] = ((position_n & 0xff0000) >> 16);
        packet->data[6] = ((position_n & 0xff00) >> 8);
        packet->data[7] = (position_n & 0xff);
        port->write(packet);
    }

    {
        // map anchor1 -- 7d9
        // lon
        auto packet = driver::CANPacket::create(0x7d9, 8);
        double offseted_lon = result->position_anchor[1] + kLongitudeOffset;
        offseted_lon = clamp(offseted_lon, 0., 360.);
        const uint64_t anchor_lon = offseted_lon / kPositionDegreeFactor;
        packet->data[0] = ((uint64_t)(anchor_lon & 0xff00000000) >> 32);
        packet->data[1] = ((uint64_t)(anchor_lon & 0xff000000) >> 24);
        packet->data[2] = ((uint64_t)(anchor_lon & 0xff0000) >> 16);
        packet->data[3] = ((uint64_t)(anchor_lon & 0xff00) >> 8);
        packet->data[4] = (anchor_lon & 0xff);

        // east
        double offseted_pose_e = result->position[0] + kPositionENUOffset;
        if (offseted_pose_e < 0.) {
            offseted_pose_e = 1.e-6;
        }
        uint32_t position_e = offseted_pose_e / kPositionMeterFactor;
        position_e <<= 2;
        packet->data[5] = ((position_e & 0xff0000) >> 16);
        packet->data[6] = ((position_e & 0xff00) >> 8);
        packet->data[7] = (position_e & 0xff);
        port->write(packet);
    }

    {
        // position variance -- 7DA
        constexpr double kPositionVarFactor = 0.001;
        auto packet = driver::CANPacket::create(0x7da, 8);
        // x
        double offseted_position_var_x = result->position_variance[0] /* + offset */;
        offseted_position_var_x = clamp(offseted_position_var_x, 0., 1048.);
        uint32_t var_x = offseted_position_var_x / kPositionVarFactor;
        var_x <<= 4;
        packet->data[0] = ((var_x & 0xff0000) >> 16);
        packet->data[1] = ((var_x & 0xff00) >> 8);
        packet->data[2] = (var_x & 0xf0);

        // y
        double offseted_position_var_y = result->position_variance[1] /* + offset */;
        offseted_position_var_y = clamp(offseted_position_var_y, 0., 1048.);
        uint32_t var_y = offseted_position_var_y / kPositionVarFactor;
        var_y <<= 4;
        packet->data[5] = ((var_y & 0xff0000) >> 16);
        packet->data[6] = ((var_y & 0xff00) >> 8);
        packet->data[7] = (var_y & 0xf0);

        // z
        double offseted_position_var_z = result->position_variance[2] /* + offset */;
        offseted_position_var_z = clamp(offseted_position_var_z, 0., 1048.);
        uint32_t var_z = offseted_position_var_z / kPositionVarFactor;
        uint8_t temp_data2_low4 = ((var_z & 0x0f0000) >> 16);
        packet->data[2] += temp_data2_low4;
        packet->data[3] = ((var_z & 0xff00) >> 8);
        packet->data[4] = (var_z & 0xff);

        port->write(packet);
    }
}
#endif
#endif

}  // namespace feedback
}  // namespace odometry
}  // namespace device
}  // namespace hera
}  // namespace wayz