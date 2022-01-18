HERA_PLUGIN_DATA_DEFINE_START(S32VSalRawImage, 0x0403)
struct S32VSalRawData {
    ///
    /// @brief timestamp of capture start, UTC, in ns
    ///
    uint64_t timestamp_capture_start_ns;
    ///
    /// @brief timestamp of capture ebd, UTC, in ns
    ///
    /// @note not implenmented yet
    uint64_t timestamp_capture_end_ns;
    data::Image::ImageMeta image_meta;  ///< meta data of image
    uint32_t image_data_size;           ///< size of image_data
    uint8_t image_data[0];              ///< raw image data
};

///
/// @brief Union to allow access by data or by bytes
///
union {
    S32VSalRawData data;                  ///< union entry: data with structure
    uint8_t buf[sizeof(S32VSalRawData)];  ///< union entry: raw buffer of bytes
};

HERA_PLUGIN_DATA_DEFINE_END

HERA_PLUGIN_DATA_DEFINE_START(S32VSalCompressedImage, 0x0404)

struct S32VSalCompressedData {
    ///
    /// @brief timestamp of capture start, UTC, in ns
    ///
    uint64_t timestamp_capture_start_ns;
    ///
    /// @brief timestamp of capture ebd, UTC, in ns
    ///
    /// @note not implenmented yet
    uint64_t timestamp_capture_end_ns;

    data::CompressedImage::CompressFormat compress_format;  ///< format of compression
    uint32_t image_data_size;                               ///< size of image_data, in bytes
    uint8_t image_data[0];                                  ///< compressed image data
};

///
/// @brief Union to allow access by data or by bytes
///
union {
    S32VSalCompressedData data;                  ///< union entry: data with structure
    uint8_t buf[sizeof(S32VSalCompressedData)];  ///< union entry: raw buffer of bytes
};
HERA_PLUGIN_DATA_DEFINE_END