HERA_PLUGIN_DATA_DEFINE_START(InstaWebcamVideoPacket, 0x0431)

uint64_t timestamp_host_ns;    ///< Host receive timestamp
uint64_t frame_index;          ///< Local frame sequence
uint32_t width;                ///< V4L2 negotiated width
uint32_t height;               ///< V4L2 negotiated height
uint32_t fourcc;               ///< V4L2 pixel format FOURCC
uint32_t payload_size;         ///< Raw frame payload size
uint8_t payload[0];            ///< Raw frame bytes (MJPG/YUYV)

HERA_PLUGIN_DATA_DEFINE_END
