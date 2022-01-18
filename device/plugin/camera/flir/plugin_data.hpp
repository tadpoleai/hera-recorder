HERA_PLUGIN_DATA_DEFINE_START(FlirCompressedImage, 0x0401)

///
/// @brief timestamp of capture, UTC, in ns
///
/// Timestamp at the half from capture triggered to shutter closed,
/// i.e, trigger timestamp + 1/2 of shutter duration
uint64_t timestamp_intrinsic_ns;

data::CompressedImage::CompressFormat compress_format;  ///< format of compression
uint32_t image_data_size;                               ///< size of image_data, in bytes
uint8_t image_data[0];                                  ///< compressed image data

HERA_PLUGIN_DATA_DEFINE_END

HERA_PLUGIN_DATA_DEFINE_START(FlirRawImage, 0x0402)

///
/// @brief timestamp of capture, UTC, in ns
///
/// Timestamp at the half from capture triggered to shutter closed,
/// i.e, trigger timestamp + 1/2 of shutter duration
uint64_t timestamp_intrinsic_ns;

data::Image::ImageMeta image_meta;  ///< meta data of image
uint32_t flir_embedded_timestamp;   ///< flir embedded timstamp, see flir's manual
uint32_t flir_embedded_shutter;     ///< flir embedded timstamp, see flir's manual
uint32_t image_data_size;           ///< size of image_data
uint8_t image_data[0];              ///< raw image data

HERA_PLUGIN_DATA_DEFINE_END
