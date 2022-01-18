HERA_PLUGIN_DATA_DEFINE_START(S32VSalData, 0x0303)
enum class QualityType : uint8_t {
    Invalid = 0u,
    SinglePoint = 1u,
    Pseudorangedifferential = 2u,
    Sensitive = 3u,
    RTKFixed = 4u,
    RTKFloating = 5u,
    Estimated = 6u,
    Manual = 7u,
    Simulation = 8u,
};

public:
///
/// @brief Data structure of GNSSS32VSalData
///
struct S32VSalDataUnion {
    uint64_t timestamp_intrinsic_ns;  ///< UTC Timestamp in ns
    double latitude;                  ///< Latitude [degree].
    double longitude;                 ///< Longitude [degree].
    double altitude;                  ///< Altitude over sea level, represented following WGS84 [m].
    double speed;                     ///< Horizontal speed [m/s].
    QualityType quality;              ///< Quality Indicators
    double pdop;                      ///< Position Dilution of Precision
    double hdop;                      ///< Horizontal Dilution of Precision
    double vdop;                      ///< Vertical Dilution of Precision
    uint8_t latitude_valid;
    uint8_t longitude_valid;
    uint8_t altitude_valid;
    uint8_t speed_valid;
    uint8_t pdop_valid;
    uint8_t hdop_valid;
    uint8_t vdop_valid;
};

///
/// @brief Union to allow access by data or by bytes
///
union {
    S32VSalDataUnion data;                  ///< union entry: data with structure
    uint8_t buf[sizeof(S32VSalDataUnion)];  ///< union entry: raw buffer of bytes
};

HERA_PLUGIN_DATA_DEFINE_END