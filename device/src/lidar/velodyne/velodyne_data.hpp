///
/// @file velodyne_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class VelodyneDeviceData
/// @version 0.1
/// @date 2019-11-07
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include "../../device.hpp"
#include "../lidar_data.hpp"
namespace wayz {
namespace hera {
namespace device {
namespace lidar {
namespace velodyne {

#pragma pack(push, 1)

///
/// @brief Device data for Velodyne Lidar, Derived from Storage Data
///
/// @anchor manual
/// @see <a href="https://shorturl.at/bnsxY" target="_blank"
/// rel="noopener noreferrer">VLP-16-User-Manual</a>
///
class VelodynePacket final : public data::DeviceData {
public:
    VelodynePacket() = delete;

public:
    ///
    /// @brief Number of DataBlocks in every Velodyne UDP Packet
    ///
    /// @see @ref manual "VLP-16-User-Manual" section: 9.3
    /// Packet Types and Definitions, page: 54 - 60
    static constexpr auto NumDataBlockPerPacket = 12;

    ///
    /// @brief Number of Channels in every DataBlock
    ///
    /// @see @ref manual "VLP-16-User-Manual" section: 9.3
    /// Packet Types and Definitions, page: 54 - 60
    /// @note Every channel contains a point of PointXYZI(Raw)
    static constexpr auto NumChannelPerDataBlock = 32;

    ///
    /// @brief Number of Points in every Velodyne UDP Packet
    ///
    static constexpr auto NumPointsPerPacket = NumChannelPerDataBlock * NumDataBlockPerPacket;

    ///
    /// @brief A single byte indicating Lidar model amont Velodyne's lidars
    /// @see @ref manual "VLP-16-User-Manual" section: 9.3
    /// Factory Bytes, page: 56
    enum class LidarType : int8_t {
        HDL32E = 0x21,  ///< Velodyne HDL-32E
        VLP16C = 0x22,  ///< Velodyne VLP-16C
        VLP32C = 0x28,  ///< Velodyne VLP-32C
    };

    ///
    /// @brief A single byte indicating return mode of lidar
    /// @see @ref manual "VLP-16-User-Manual" section: 9.3
    /// Factory Bytes, page: 56
    /// @note DualReturn is not supplied in this driver
    enum class ReturnMode : uint8_t {
        Strongest = 0x37,   ///< Stronest Candidate Point
        LastReturn = 0x38,  ///< Last (Farest) Candidate Point
        DualReturn = 0x39   ///< Dual Return
    };

    ///
    /// @brief Data structure of Channel (aka point)
    ///
    struct ChannelData {
        uint16_t distance;     ///< distance from photocentre, granularity various
        uint8_t reflectivity;  ///< relative reflectivity of point, defined by Velodyne
                               /// ranges [0-255]
    };

    ///
    /// @brief Data structure of DataBlock
    ///
    struct DataBlock {
        uint16_t flag;     ///< unused flag, 2 bytes
        uint16_t azimuth;  ///< azimuth, in hundredth of degree, i.e., 35999 represents 359.99 deg
                           /// from Velodyne's Coordinate X, CW,
        ChannelData channels[NumChannelPerDataBlock];  ///< array of Channel datas
    };

    ///
    /// @brief Data structure of one Velodyne UDP Packet
    ///
    /// @see @ref manual "VLP-16-User-Manual" section: 9.3
    /// Packet Types and Definitions, page: 54 - 60
    struct VelodynePacketUnion {
        DataBlock data_blocks[NumDataBlockPerPacket];  ///< array of DataBlocks
        uint32_t timestamp;                            ///< internal timestamp, in us
                                                       /// from the start of this hour
        ReturnMode return_mode;                        ///< return mode
        LidarType lidar_type;                          ///< lidar model type
    };

    ///
    /// @brief Union to allow access by data or by bytes
    ///
    union {
        VelodynePacketUnion data;                  ///< union entry: data with structure
        uint8_t buf[sizeof(VelodynePacketUnion)];  ///< union entry: raw buffer of bytes
    };
};

#pragma pack(pop)

}  // namespace velodyne
}  // namespace lidar
}  // namespace device
}  // namespace hera
}  // namespace wayz