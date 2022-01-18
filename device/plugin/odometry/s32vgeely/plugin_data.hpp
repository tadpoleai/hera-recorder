HERA_PLUGIN_DATA_DEFINE_START(S32VGeelyCANFrame, 0x0601)

///
/// @brief Device data for S32VGeely Series Car, Derived from Storage Data
///
struct S32VGeelyCANPacket {
    uint64_t timestamp_ns;  ///< Timestamp given by can driver
    uint32_t id_can;        ///< CAN ID (Arbitration Id) of the message sender.
    uint16_t dlc_can;       ///< CAN DLC, number of bytes of the payload.
    uint8_t data_can[0];    ///< CAN Packet payload.
};

///
/// @brief Union to allow access by data or by bytes
///
union {
    S32VGeelyCANPacket data;                  ///< union entry: data with structure
    uint8_t buf[sizeof(S32VGeelyCANPacket)];  ///< union entry: raw buffer of bytes
};

HERA_PLUGIN_DATA_DEFINE_END