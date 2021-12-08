///
/// @file gnss_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Derived classes of SensorData for GNSS
/// @version 0.1
/// @date 2019-11-22
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "../sensor_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

#pragma pack(push, 1)

///
/// @brief SensorData for GNSS NMEA
///
class NavSatFix final : public SensorData {
public:
    ///
    /// @brief GNSSFixType
    ///
    /// Navigation Satellite fix status for any Global Navigation Satellite System
    /// Whether to output an augmented fix is determined by both the fix
    /// type and the last time differential corrections were received.  A
    /// fix is valid when status >= STATUS_FIX.
    ///
    /// @note (deprecated)
    /// The definition above is defined by ROS.
    /// due to compatibility issues, only 'FIX' and 'NO_Fix' is used in Wayz's
    /// backend codes(@gaia)_. so set this to 'FIX' when result is valid, otherwise,
    /// set to 'NO_Fix'
    ///
    /// @note Update
    /// Since version 4.6.5, the StatusType should be interpreted as follow
    /// NO_Fix = No solution
    /// FIX = Single Point / DGPS
    /// SBAS_Fix = Floating Solution of RTK-GPS
    /// GBAS_Fix = Fixed Solution of RTK-GPS
    ///
    enum class StatusType : int8_t {
        NO_Fix = -1,   ///< unable to fix position
        FIX = 0,       ///< unaugmented fix
        SBAS_Fix = 1,  ///< with satellite-based augmentation
        GBAS_FIX = 2,  ///< with ground-based augmentation
    };

    ///
    /// @brief GNSSServiceType
    ///
    /// Bits defining which Global Navigation Satellite System signals were
    /// used by the receiver
    ///
    /// Unused in current version of backend (@gaia)
    enum class ServiceType : uint16_t {
        GPS = 1,
        GLONASS = 2,
        COMPASS = 4,  ///< includes BeiDou
        GALILEO = 8,
    };

    ///
    /// @brief Satellite fix status information
    ///
    struct NavSatStatus {
        StatusType status;
        ServiceType service;
    };

    ///
    /// @brief Enum of position covariance type
    ///
    enum class PositionCovarianceType : uint8_t { Unknown = 0, Approximated = 1, DiagonalKnown = 2, Known = 3 };

public:
    NavSatStatus status;  ///< satellite fix status information

    ///
    /// @brief Latitude [degrees]. Positive is north of equator; negative is south.
    /// (quiet NaN not available).
    ///
    double latitude;

    ///
    /// @brief Longitude [degrees]. Positive is east of prime meridian; negative is west.
    /// (quiet NaN not available).
    ///
    double longitude;

    ///
    /// @brief Altitude [m]. Positive is above the WGS 84 ellipsoid
    /// (quiet NaN if no altitude is available).
    double altitude;

    ///
    /// @brief Position covariance [m^2] defined relative to a tangential plane
    ///
    /// through the reported position. The components are East, North, and
    /// Up (ENU), in row-major order.
    ///
    /// Beware: this coordinate system exhibits singularities at the poles.
    double position_covariance[9];

    ///
    /// @brief Position covariance type
    ///
    /// If the covariance of the fix is known, fill it in completely. If the
    /// GNSS receiver provides the variance of each measurement, put them
    /// along the diagonal. If only Dilution of Precision is available,
    // estimate an approximate covariance from that.
    PositionCovarianceType position_covariance_type;

    int32_t num_satellites;  ///< Number of satellites in use
};

#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz
