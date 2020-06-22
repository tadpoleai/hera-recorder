///
/// @file crc32.h
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-06-04
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <stdint.h>

namespace wayz {
namespace hera {
namespace device {
namespace driver {

///
/// @brief Calculates the CRC-32 of a block of data all at once
///
/// @param ulCount Number of bytes in the data block
/// @param char* Data block
/// @return uint32_t Result
///
uint32_t CalculateBlockCRC32(uint32_t ulCount, unsigned char* ucBuffer);

///
/// @brief Calculates the CRC-16/Modbus of a block of data all at once
///
/// @param wdCount Number of bytes in the data block
/// @param char* Data block
/// @return uint16_t Result
///
uint16_t CalculateBlockCRC16(uint16_t wdCount, unsigned char* ucBuffer);

}  // namespace driver
}  // namespace device
}  // namespace hera
}  // namespace wayz