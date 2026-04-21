HERA_PLUGIN_DATA_DEFINE_START(InstaVideoPacket, 0x0421)

int64_t timestamp_device_ns;   ///< Timestamp provided by Insta SDK callback
uint64_t timestamp_host_ns;    ///< Host receive timestamp
uint8_t stream_type;           ///< SDK stream type
int32_t stream_index;          ///< Lens stream index
uint32_t payload_size;         ///< Raw encoded payload size
uint8_t payload[0];            ///< Raw H264/H265 bytes

HERA_PLUGIN_DATA_DEFINE_END

HERA_PLUGIN_DATA_DEFINE_START(InstaGyroPacket, 0x0422)

uint64_t timestamp_host_ns;    ///< Host receive timestamp
uint32_t sample_count;         ///< Number of gyro samples in payload
uint32_t payload_size;         ///< Raw gyro payload bytes
uint8_t payload[0];            ///< Raw bytes copied from std::vector<GyroData>

HERA_PLUGIN_DATA_DEFINE_END
