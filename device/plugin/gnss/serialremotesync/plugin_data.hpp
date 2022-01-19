HERA_PLUGIN_DATA_DEFINE_START(SerialRemotesyncNmea, 0x0304)

///
/// @brief Device data for SerialRemotesync's Serial Data containing NMEA Sentence, etc.
///

struct SerialRemotesyncNmeaUnion {
    uint8_t timestamp_valid;       ///< boolean, indicating if timestamp_intrinsic_ns is valid
    uint64_t timestamp_ns;         ///< valid timestamp_ns in reference time coordinatem, if timestamp_valid is true
    uint8_t nmea_sentence_length;  ///< length of nmea_sentence, in bytes
    uint8_t nmea_sentence[];       ///< NMEA Sentence, full
};

///
/// @brief Union to allow access by data or by bytes
///
union {
    SerialRemotesyncNmeaUnion data;                  ///< union entry: data with structure
    uint8_t buf[sizeof(SerialRemotesyncNmeaUnion)];  ///< union entry: raw buffer of bytes
};

HERA_PLUGIN_DATA_DEFINE_END