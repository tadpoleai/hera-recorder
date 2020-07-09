///
/// @file ins_data.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Derived classes of SensorData for INS
/// @date 2020-06-03
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "../device_data.hpp"

#ifdef HERA_COMPILE_IN_REPO
#include "common/include/third_party/enum.hpp"
#else
#include <hera/common/third_party/enum.hpp>
#endif

namespace wayz {
namespace hera {
namespace device {
namespace data {

/// @anchor novatel
/// @see <a href="https://docs.novatel.com/OEM7/Content/PDFs/OEM7_Commands_Logs_Manual.pdf" target="_blank"
/// rel="noopener noreferrer">OEM7_Commands_Logs_Manual</a>

#pragma pack(push, 1)

///
/// @brief Solution Status
///
/// @see @ref novatel Table 73 Page 396
///
BETTER_ENUM(SolutionStatus,
            uint32_t,
            SOL_COMPUTED = 0u,  ///< Solution computed
            INSUFFICIENT_OBS = 1u,
            NO_CONVERGENCE = 2u,
            SINGULARITY = 3u,
            COV_TRACE = 4u,
            TEST_DIST = 5u,
            COLD_START = 6u,
            V_H_LIMIT = 7u,
            VARIANCE = 8u,
            RESIDUALS = 9u,
            INTEGRITY_WARNING = 13u,
            PENDING = 18u,
            INVALID_FIX = 19u,
            UNAUTHORIZED = 20u,
            INVALID_RATE = 22u);

///
/// @brief Inertial Solution Status
///
/// @see @ref novatel Table 171 Page 865
///
BETTER_ENUM(
        InertialSolutionStatus,
        uint32_t,
        INS_INACTIVE = 0u,  ///< IMU logs are present, but the alignment routine has not started; INS is inactive

        INS_ALIGNING,  ///< INS is in alignment mode

        INS_HIGH_VARIANCE,  ///< The INS solution is in navigation mode but the azimuth solution uncertainty has
                            ///< exceeded
                            /// the threshold. The default threshold is 2 degrees for most IMUs.1 The solution is still
                            /// valid but you should monitor the solution uncertainty in the INSSTDEV log (see page 897)
                            /// You may encounter this state during times when the GNSS, used to aid the INS, is absent

        INS_SOLUTION_GOOD,  ///< The INS filter is in navigation mode and the INS solution is go

        INS_SOLUTION_FREE,  ///< The INS filter is in navigation mode and the GNSS solution is suspected to be in error
                            /// This may be due to multipath or limited satellite visibility. The inertial filter has
                            /// rejected the GNSS position and is waiting for the solution quality to improve

        INS_ALIGNMENT_COMPLETE,  ///< The INS filter is in navigation mode, but not enough vehicle dynamicshave been
                                 /// experienced for the system to be within specification

        DETERMINING_ORIENTATION,  ///< INS is determining the IMU axis aligned with gravity.

        WAITING_INITIALPOS,  ///< The INS filter has determined the IMU orientation and is awaiting an initial position
                             ///< estimate to begin the alignment process

        WAITING_AZIMUTH,  ///< The INS filer has orientation, initial biases, initial position and valid roll/pitch
                          ///< estimated. Will not proceed until initial azimuth is entered

        INITIALIZING_BIASES,  ///< The INS filter is estimating initial biases during the first 10 seconds of stationary
                              ///< data

        MOTION_DETECT  ///< The INS filter has not completely aligned, but has detected motion
);

///
/// @brief Position or Velocity Type
///
/// @see @ref novatel Table 74 Page 397
///
BETTER_ENUM(
        PositionVelocityType,
        uint32_t,
        UNKNOWN = 999u,

        NONE = 0u,         ///< No solution
        FIXEDPOS = 1u,     ///< Position has been fixed by the FIX position command or byposition averaging.
        FIXEDHEIGHT = 2u,  ///< Position has been fixed by the FIX height or FIX auto commandor by position averaging
        FLOATCONV = 4u,    ///< Solution from floating point carrier phase ambiguities
        WIDELANE = 5u,     ///< Solution from wide-lane ambiguities
        NARROWLANE = 6u,   ///< Solution from narrow-lane ambiguities
        DOPPLER_VELOCITY = 8u,  ///< Velocity computed using instantaneous Doppler
        SINGLE = 16u,           ///< Single point position
        PSRDIFF = 17u,          ///< Pseudorange differential solution
        WAAS = 18u,             ///< Solution calculated using corrections from an SBAS
        PROPAGATED = 19u,       ///< Propagated by a Kalman filter without new observations
        L1_FLOAT = 32u,         ///< Floating L1 ambiguity solution
        IONOFREE_FLOAT = 33u,   ///< Floating ionospheric-free ambiguity solution
        NARROW_FLOAT = 34u,     ///< Floating narrow-lane ambiguity solution
        L1_INT = 48u,           ///< Integer L1 ambiguity solution
        WIDE_INT = 49u,         ///< Integer wide-lane ambiguity solution
        NARROW_INT = 50u,       ///< Integer narrow-lane ambiguity solution

        RTK_DIRECT_INS = 51u,           ///< RTK status where the RTK filter is directly initialized from the INS filter
        INS_SBAS = 52u,                 ///< INS calculated position corrected for the antenna
        INS_PSRSP = 53u,                ///< INS pseudorange single point solution – no DGPS corrections
        INS_PSRDIFF = 54u,              ///< INS pseudorange differential solution
        INS_RTKFLOAT = 55u,             ///< INS RTK floating point ambiguities solution
        INS_RTKFIXED = 56u,             ///< INS RTK fixed ambiguities solution
        PPP_CONVERGING = 68u,           ///< Converging TerraStar-C solution
        PPP = 69u,                      ///< Converged TerraStar-C solution
        OPERATIONAL = 70u,              ///< Solution accuracy is within UAL operational limit
        WARNING = 71u,                  ///< Solution accuracy is outside UAL operational limit but withinwarning limit
        OUT_OF_BOUNDS = 72u,            ///< Solution accuracy is outside UAL limits
        INS_PPP_CONVERGING = 73u,       ///< INS NovAtel CORRECT Precise Point Positioning (PPP) solutionconverging
        INS_PPP = 74u,                  ///< INS NovAtel CORRECT PPP solution
        PPP_BASIC_CONVERGING = 77u,     ///< Converging TerraStar-L solution
        PPP_BASIC = 78u,                ///< Converged TerraStar-L solution
        INS_PPP_BASIC = 79u,            ///< INS NovAtel CORRECT PPP basic solution
        INS_PPP_BASIC_CONVERGING = 80u  ///< INS NovAtel CORRECT PPP basic solution converging
);

class BestPosition final : public SensorData {
public:
    SolutionStatus solution_status;

    PositionVelocityType position_type;

    ///
    /// @brief Latitude [degrees]. Positive is north of equator; negative is south.
    ///
    double latitude;

    ///
    /// @brief Longitude [degrees]. Positive is east of prime meridian; negative is west.
    ///
    double longitude;

    ///
    /// @brief Altitude [m]. Positive is above the WGS 84 ellipsoid
    /// @note Height + Undulation
    /// (quiet NaN if no altitude is available).
    double altitude;

    double latitude_deviation;   ///< [m] Latitude standard deviation
    double longitude_deviation;  ///< [m] Longitude standard deviation
    double altitude_deviation;   ///< [m] Altitude standard deviation
};

///
/// @brief Corrected IMU Data
///
/// The CORRIMUDATA log contains the RAWIMU data corrected for gravity, the earth’s rotation and estimated sensor
/// errors.
/// @see @ref novatel 5.4 CORRIMUDATA Page 851
///
/// @note in manual the values are incremental values, but here they are all in SI unit.
///
class CorrectedImu final : public SensorData {
public:
    double angular_velocity[3];     ///< array of 3-axis angular velocity, in rad/s, CCW, right-handed
    double linear_acceleration[3];  ///< array of 3-axis linear acceleration, in m/s^2, right-handed
    double magnetic_field[3];       ///< array of 3-axis magnetic field, in Tesla, right-handed
};

///
/// @brief Position from INS
///
class InsPosition final : public SensorData {
public:
    InertialSolutionStatus ins_status;

    PositionVelocityType position_type;

    ///
    /// @brief Latitude [degrees]. Positive is north of equator; negative is south.
    ///
    double latitude;

    ///
    /// @brief Longitude [degrees]. Positive is east of prime meridian; negative is west.
    ///
    double longitude;

    ///
    /// @brief Altitude [m]. Positive is above the WGS 84 ellipsoid
    /// @note Height + Undulation
    /// (quiet NaN if no altitude is available).
    double altitude;

    double latitude_deviation;   ///< [m] Latitude standard deviation
    double longitude_deviation;  ///< [m] Longitude standard deviation
    double altitude_deviation;   ///< [m] Altitude standard deviation
};

#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz