HERA_PLUGIN_DATA_DEFINE_START(AceinnaData, 0x0201)

///
/// @brief Data structure of one packet of Aceinna
///
/// The packet is sent by Wayz Tron Sync Board's serial output, on SerialMsgType  0
///
struct AceinnaDataUnion {
    uint64_t timestamp;   ///< timestamp of 'DataReady' pin's falling edge, UTC, in ns
    int16_t gyro[3];      ///< array of Gyro raw data
    int16_t accel[3];     ///< array of Accel raw data
    int16_t magnetic[3];  ///< array of Magnetic raw data
};

///
/// @brief Union to allow access by data or by bytes
///
union {
    AceinnaDataUnion data;                  ///< union entry: data with structure
    uint8_t buf[sizeof(AceinnaDataUnion)];  ///< union entry: raw buffer of bytes
};

HERA_PLUGIN_DATA_DEFINE_END