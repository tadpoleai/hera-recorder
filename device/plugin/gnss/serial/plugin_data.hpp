HERA_PLUGIN_DATA_DEFINE_START(SerialNmea, 0x0302)

///
/// @brief Data structure of NMEA Sentence packet
///
struct SerialNmeaUnion {
    uint32_t nmea_sentence_length;  ///< length of nmea_sentence, in bytes
    uint8_t nmea_sentence[];        /// NMEA Sentence, (usually) without trailing <LF>
};

///
/// @brief Union to allow access by data or by bytes
///
union {
    SerialNmeaUnion data;                  ///< union entry: data with structure
    uint8_t buf[sizeof(SerialNmeaUnion)];  ///< union entry: raw buffer of bytes
};

HERA_PLUGIN_DATA_DEFINE_END