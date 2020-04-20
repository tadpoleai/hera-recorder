///
/// @file feedback.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Feed LocalizationResult back
/// @date 2020-04-20
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "feedback.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace odometry {
namespace feedback {

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VCAN

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
        const uint16_t heading = result->orientation[0] / kOrientationFactor;
        const uint16_t pitch = result->orientation[1] / kOrientationFactor;
        const uint16_t roll = result->orientation[2] / kOrientationFactor;
        packet->data[0] = (heading >> 8);
        packet->data[1] = (heading && 0xff);
        packet->data[2] = (pitch >> 8);
        packet->data[3] = (pitch && 0xff);
        packet->data[4] = (roll >> 8);
        packet->data[5] = (roll && 0xff);
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

    {
        // position1 -- 7D5
        auto packet = driver::CANPacket::create(0x7d5, 8);

        double offseted_position_l = result->position[4];
        if (offseted_position_l < 0.) {
            offseted_position_l += 360.;
        }
        const uint64_t position_l = offseted_position_l / kPositionDegreeFactor;
        packet->data[3] = ((uint64_t)(position_l && 0xff00000000) >> 32);
        packet->data[4] = ((uint64_t)(position_l && 0xff000000) >> 24);
        packet->data[5] = ((uint64_t)(position_l && 0xff0000) >> 16);
        packet->data[6] = ((uint64_t)(position_l && 0xff00) >> 8);
        packet->data[7] = (position_l && 0xff);
        port->write(packet);
    }

    {
        // position2 -- 7d6
        auto packet = driver::CANPacket::create(0x7d6, 8);
        double offseted_pose_h = result->position[2] + 2097.;
        if (offseted_pose_h < 0.) {
            offseted_pose_h = 1.e-6;
        }
        uint32_t position_h = offseted_pose_h / kPositionMeterFactor;
        position_h <<= 2;
        packet->data[0] = ((position_h && 0xff0000) >> 16);
        packet->data[1] = ((position_h && 0xff00) >> 8);
        packet->data[2] = (position_h && 0xff);

        double offseted_position_b = result->position[3];
        if (offseted_position_b < 0.) {
            offseted_position_b += 360.;
        }
        const uint64_t position_b = offseted_position_b / kPositionDegreeFactor;
        packet->data[3] = ((uint64_t)(position_b && 0xff00000000) >> 32);
        packet->data[4] = ((uint64_t)(position_b && 0xff000000) >> 24);
        packet->data[5] = ((uint64_t)(position_b && 0xff0000) >> 16);
        packet->data[6] = ((uint64_t)(position_b && 0xff00) >> 8);
        packet->data[7] = (position_b && 0xff);
        port->write(packet);
    }

    {
        // map anchor -- 7d7
        auto packet = driver::CANPacket::create(0x7d7, 8);
        double offseted_pose_n = result->position[1] + 2097.;
        if (offseted_pose_n < 0.) {
            offseted_pose_n = 1.e-6;
        }
        uint32_t position_n = offseted_pose_n / kPositionMeterFactor;
        position_n <<= 2;
        packet->data[5] = ((position_n && 0xff0000) >> 16);
        packet->data[6] = ((position_n && 0xff00) >> 8);
        packet->data[7] = (position_n && 0xff);
        port->write(packet);
    }

    {
        // map anchor1 -- 7d9
        auto packet = driver::CANPacket::create(0x7d9, 8);
        double offseted_pose_e = result->position[0] + 2097.;
        if (offseted_pose_e < 0.) {
            offseted_pose_e = 1.e-6;
        }
        uint32_t position_e = offseted_pose_e / kPositionMeterFactor;
        position_e <<= 2;
        packet->data[5] = ((position_e && 0xff0000) >> 16);
        packet->data[6] = ((position_e && 0xff00) >> 8);
        packet->data[7] = (position_e && 0xff);
        port->write(packet);
    }

    // {
    //     // position variance -- 7DA
    //     auto packet = driver::CANPacket::create(0x7d9, 8);
    //     // todo
    // }
}
#endif
#endif

}  // namespace feedback
}  // namespace odometry
}  // namespace device
}  // namespace hera
}  // namespace wayz