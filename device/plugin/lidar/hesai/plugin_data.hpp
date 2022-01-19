HERA_PLUGIN_DATA_DEFINE_START(HesaiPacket, 0x0511)

enum class LidarType : int8_t {
    PandarXT32 = 0x42,  ///< Hesai PandarXT
};

///
/// @brief A single byte indicating return mode of lidar
///
/// @note As same as Velodyne
enum class ReturnMode : uint8_t {
    Strongest = 0x37,   ///< Stronest Candidate Point
    LastReturn = 0x38,  ///< Last (Farest) Candidate Point
    DualReturn = 0x39   ///< Dual Return
};

struct DataTime {
    uint8_t year;  ///< Current year minus 1900
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

///
/// @brief Data structure of Channel (aka point)
///
struct ChannelData {
    uint16_t distance;     ///< distance from photocentre, granularity various
    uint8_t reflectivity;  ///< relative reflectivity of point, defined by Velodyne
                           /// ranges [0-255]
    uint8_t reserved;      ///< Reserved byte
};

///
/// @brief Data structure of DataBlock
///
struct DataBlock {
    uint16_t azimuth;  ///< azimuth, in hundredth of degree, i.e., 35999 represents 359.99 deg, CW
    ChannelData channels[NumChannelPerDataBlock];  ///< array of Channel datas
};

struct RawPacket {
    ///
    /// @brief PreHeader
    ///
    uint16_t sop;               ///< Start of Packet, 0xFFEE
    uint16_t protocol_version;  ///< 0x0106 For PandarXT
    uint16_t reserved;

    ///
    /// @brief Header
    ///
    uint8_t laser_num;           ///< 0x20 (32 channels);
    uint8_t block_num;           ///< 0x08 (8 blocks per packet);
    uint8_t first_block_return;  ///< The first block in this data packet
                                 /// 0x00 - Single Return
                                 /// 0x01 - Last Return in Dual Return Mode
    uint8_t dis_unit;            ///< 0x04 (4mm)
    uint8_t return_number;       ///< Number of returns that each channel generates
                                 /// 0x01 - one return
                                 /// 0x02 - two returns
    uint8_t udp_sequence_flag;   ///< LSB shows whether this packet includes a UDP sequence number field

    ///
    /// @brief Body
    ///
    DataBlock data_blocks[NumDataBlockPerPacket];  ///< array of DataBlocks

    ///
    /// @brief Tail
    ///
    uint8_t reserved_1[9];
    uint8_t high_temp_flag;     ///< 0x01 for high temperature; 0x00 for normal operation
                                /// · When high temperature is detected, the shutdown flag will be set to 0x01, and the
                                /// system will shut down after 60 s. The flag remains 0x01 during the 60 s and the
                                /// shutdown period.
                                /// · When the system is no longer in high temperature status, the
                                /// shutdown flag will be reset to 0x00 and the system will automatically return to
                                /// normal operation.
    ReturnMode return_mode;     ///< Return mode (Lidar mode)
    uint16_t motor_speed;       ///< Motor speed [rpm]
    DataTime date_time;         ///< Date time of current packet
    uint32_t timestamp_subsec;  ///< Date time of current packet, sub second part [us]
    LidarType lidar_type;       ///< 0x42 for PandarXT, a.k.a. Factory Information

    ///
    /// @brief Appendix
    ///
    uint32_t udp_sequence;
};

///
/// @brief Union to allow access by data or by bytes
///
union {
    RawPacket data;                  ///< union entry: data with structure
    uint8_t buf[sizeof(RawPacket)];  ///< union entry: raw buffer of bytes
};

HERA_PLUGIN_DATA_DEFINE_END

using HesaiPacketFullSynced = HesaiPacket;

HERA_PLUGIN_DATA_DEFINE_START(HesaiPacketUnSynced, 0x0512)
HERA_PLUGIN_DATA_DEFINE_END

HERA_PLUGIN_DATA_DEFINE_START(HesaiPacketLocalSynced, 0x0513)
HERA_PLUGIN_DATA_DEFINE_END
