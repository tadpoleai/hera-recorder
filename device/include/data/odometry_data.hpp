///
/// @file odometry_data.hpp
/// @author chunchen.wang (chunchen.wang@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2020-04-08
///
/// Copyright 2018 Wayz.ai. All Rights Reserved.
///
///
#pragma once

#include "../sensor_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

#pragma pack(push, 1)

///
/// @brief SensorData for odometry FrongWheelSpeedAngle
///
class FrontWheelSpeed final : public SensorData {
public:
    double left;   ///< speed of left front wheel
    double right;  ///< speed of right front wheel
};

///
/// @brief SensorData for odometry RearWheelSpeedAngle
///
class RearWheelSpeed final : public SensorData {
public:
    double left;   ///< speed of left rear wheel
    double right;  ///< speed of right rear wheel
};

///
/// @brief SensorData for odometry SteeringAngle
///
class SteeringAngle final : public SensorData {
public:
    double steering_angle;  ///< steering angle (degree)
};

///
/// @brief Single Axis Orientation
///
class Orientation final : public SensorData {
public:
    double orientation;  ///< orientation of axis, rad
};

///
/// @brief LocalizationResult odometry geely / chery to send back by driver
///
class LocalizationResult final : public SensorData {
public:
    static constexpr uint32_t IPCKeyS32VGeely = 0x100u;  ///< Key for sending data back to driver Geely
    static constexpr uint32_t IPCKeyS32VChery = 0x101u;  ///< Key for sending data back to driver Chery
    static constexpr uint32_t IPCElementSize = 0x100u;   ///< Element size of ipc slot
    static constexpr uint32_t IPCNumElement = 0x10u;     ///< Queue length of ipc slot

public:
    enum class SystemStatus : uint8_t { Off = 0x00u, On = 0x01u, Failed = 0x02u, Active = 0x03u };

    enum class ResultType : uint8_t {
        DataInvalid = 0x00u,
        VisionDataValid = 0x01u,
        RtkDataValid = 0x02u,
        FusionDataValid = 0x03u
    };

public:
    double position_anchor[3];  ///< Reference anchor of position[5],
                                /// [0] Latitude [degrees]. Positive is north of equator; negative is south.,
                                /// [1] Longitude [degrees]. Positive is east of prime meridian; negative is west.,
                                /// [2] Altitude [m]. Positive is above the WGS 84 ellipsoid.

    double position[5];  ///< [0:3] Position in reference to position_anchor,
                         /// [0] East [m],
                         /// [1] North [m],
                         /// [2] Upward [m]
                         /// [3:5] Position in WGS 84,
                         /// [3] Latitude [degrees]. Positive is north of equator; negative is south.,
                         /// [4] Longitude [degrees]. Positive is east of prime meridian; negative is west.

    double position_variance[3];  ///< Variance of position (principal diagonal of covariance matrix),
                                  /// [0] East [m^2],
                                  /// [1] North [m^2],
                                  /// [2] Upward [m^2]

    double orientation[3];  ///< Orientation in ENU coordinate in reference to position_anchor,
                            /// [0] Roll [rad],
                            /// [1] Pitch [rad],
                            /// [2] Yaw [rad]

    double orientation_variance[3];  ///< Variance of orientation (principal diagonal of covariance matrix),
                                     /// [0] Roll [rad^2],
                                     /// [1] Pitch [rad^2],
                                     /// [2] Yaw [rad^2]

    double orientation_covariance[3];  ///< Covariance of orientation,
                                       /// [0] Roll-Pitch [rad^2], covariance matrix[1, 2],
                                       /// [1] Roll-Yaw [rad^2], covariance matrix[1, 3],
                                       /// [2] Pitch-Yaw [rad^2], covariance matrix[2, 3],

    SystemStatus system_status;  ///< Status of Localization Service

    ResultType result_type;  ///< Type of Result
};

#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz