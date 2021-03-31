///
/// @file plugin_data.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-08-19
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "data/ins_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace ins {
namespace novatelspan {

#pragma pack(push, 1)

struct BESTPOS {
    data::SolutionStatus solution_status;
    data::PositionVelocityType position_type;
    double latitude;
    double longitude;
    double height;
    float undulation;
    uint32_t datum_id;
    float latitude_deviation;
    float longitude_deviation;
    float height_deviation;
    char base_station_id[4];
    float differential_age_sec;
    float solution_age_sec;
    uint8_t satellites_tracked_number;
    uint8_t satellites_used_number;
    uint8_t satellites_l1_number;
    uint8_t satellites_multi_number;
    uint8_t reserved;
    uint8_t extended_solution_status;
    uint8_t galileo_and_beidou_signal_mask;
    uint8_t gps_and_glonass_signal_mask;
};

struct INSPOS {
    uint32_t week;
    double seconds_into_week;
    double latitude;
    double longitude;
    double height;
    data::InertialSolutionStatus ins_status;
};

struct INSPOSX {
    data::InertialSolutionStatus ins_status;
    data::PositionVelocityType position_type;
    double latitude;
    double longitude;
    double height;
    float undulation;
    float latitude_deviation;
    float longitude_deviation;
    float height_deviation;
    uint32_t extended_solution_status;
    uint16_t time_since_update;
};

struct CORRIMUDATA {
    uint32_t gnss_week;
    double gnss_ms;
    double rotational_speed_sample[3];
    double acceleration_sample[3];
};

enum class MessageIdType : uint16_t { BESTPOS = 42u, INSPOS = 265u, INSPOSX = 1459u, CORRIMUDATA = 812u };

///
/// @brief Data for recording NovatelSpan
///
class NovatelSpanBinaryData final : public data::DeviceData {
public:
    NovatelSpanBinaryData() = delete;

public:
    ///
    /// @brief @see NovatelSpan
    ///
    uint64_t shiftation_timestamp;
    uint64_t shifation_value_ns;

    ///
    /// @brief Data structure of one packet of Novatel
    ///
    struct NovatelBinaryStruct {
        /// Header
        uint8_t sync[3];
        uint8_t header_length;
        MessageIdType message_id;
        uint8_t message_type;
        uint8_t port_address;
        uint16_t message_length;
        uint16_t sequence;
        uint8_t idle_time;
        uint8_t time_status;
        uint16_t gnss_week;
        uint32_t gnss_ms;
        uint32_t receiver_status;
        uint16_t reserved;
        uint16_t receiver_sw_version;

        // Payload
        uint8_t payload[0];
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        NovatelBinaryStruct novatel_header;                ///< union entry: data with structure
        uint8_t novatel_buf[sizeof(NovatelBinaryStruct)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

}  // namespace velodyne
}  // namespace lidar
}  // namespace device
}  // namespace hera
}  // namespace wayz
