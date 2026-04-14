HERA_PLUGIN_DATA_DEFINE_START(LivoxPacket, 0x0521)

uint64_t timestamp_device_ns;  ///< Device timestamp from Livox packet
uint64_t timestamp_host_ns;    ///< Host receive timestamp

uint32_t handle;               ///< Livox handle
uint8_t livox_dev_type;        ///< Livox device type
uint8_t livox_data_type;       ///< Livox point data type
uint8_t livox_time_type;       ///< Livox time type
uint8_t frame_cnt;             ///< Frame counter from packet
uint16_t dot_num;              ///< Point count in packet
uint16_t packet_length;        ///< Packet length field from Livox header

uint32_t payload_size;         ///< Raw payload size in bytes
uint8_t payload[0];            ///< Raw payload bytes

HERA_PLUGIN_DATA_DEFINE_END

using LivoxPacketFullSynced = LivoxPacket;

HERA_PLUGIN_DATA_DEFINE_START(LivoxPacketUnSynced, 0x0522)
HERA_PLUGIN_DATA_DEFINE_END

HERA_PLUGIN_DATA_DEFINE_START(LivoxPacketLocalSynced, 0x0523)
HERA_PLUGIN_DATA_DEFINE_END

HERA_PLUGIN_DATA_DEFINE_START(LivoxImuPacket, 0x0524)

uint64_t timestamp_device_ns;  ///< Device timestamp from Livox packet
uint64_t timestamp_host_ns;    ///< Host receive timestamp

uint32_t handle;               ///< Livox handle
uint8_t livox_dev_type;        ///< Livox device type
uint8_t livox_time_type;       ///< Livox time type

float gyro_x;
float gyro_y;
float gyro_z;
float acc_x;
float acc_y;
float acc_z;

uint32_t payload_size;         ///< Raw IMU payload size in bytes
uint8_t payload[0];            ///< Raw IMU payload bytes

HERA_PLUGIN_DATA_DEFINE_END
