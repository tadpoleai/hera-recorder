HERA_PLUGIN_DATA_DEFINE_START(AEHaitaiData, 0x0611)

///
/// @brief Device data for AEHaitaiData, Derived from Storage Data
///

enum class FrameType : uint8_t { ENCODER_ANGLE = 0x01u };

struct EncoderAngle {
    uint8_t angle_h8;
    uint8_t angle_l8;
    uint8_t direction;
};

struct AEHaitaiDataUnion {
    uint8_t header;
    uint8_t address;
    FrameType frame_type;
    uint8_t frame_data[0];
};

///
/// @brief Union to allow access by data or by bytes
///
union {
    AEHaitaiDataUnion data;                  ///< union entry: data with structure
    uint8_t buf[sizeof(AEHaitaiDataUnion)];  ///< union entry: raw buffer of bytes
};

HERA_PLUGIN_DATA_DEFINE_END