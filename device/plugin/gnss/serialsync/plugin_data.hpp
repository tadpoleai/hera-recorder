HERA_PLUGIN_DATA_DEFINE_START(SerialSyncNmea, 0x0301)

///
/// @brief Data structure of NMEA Sentence packet
///
struct SerialSyncNmeaUnion {
    ///
    /// @brief time::Timestamp of message, UTC, in ns
    ///
    /// Obtained by calculating from NMEA Sentence and time shiftation
    ///
    uint64_t timestamp_intrinsic_ns;
    uint8_t timestamp_valid;        ///< boolean, indicating if timestamp_intrinsic_ns is valid
    uint32_t nmea_sentence_length;  ///< length of nmea_sentence, in bytes
    uint8_t nmea_sentence[];        /// NMEA Sentence, (usually) without trailing <LF>
};

///
/// @brief Union to allow access by data or by bytes
///
union {
    SerialSyncNmeaUnion data;                  ///< union entry: data with structure
    uint8_t buf[sizeof(SerialSyncNmeaUnion)];  ///< union entry: raw buffer of bytes
};

HERA_PLUGIN_DATA_DEFINE_END