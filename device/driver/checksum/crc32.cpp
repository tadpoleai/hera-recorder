///
/// @file crc32.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-06-04
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "crc32.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace driver {

///
/// @brief Calculate a CRC value to be used by CRC calculation functions.
///
static uint32_t CRC32Value(uint32_t i)
{
    static constexpr uint32_t CRC32_POLYNOMIAL = 0xEDB88320L;
    uint32_t j;
    uint32_t ulCRC;
    ulCRC = i;
    for (j = 8; j > 0; j--) {
        if (ulCRC & 1)
            ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
        else
            ulCRC >>= 1;
    }
    return ulCRC;
}

///
/// @brief Calculates the CRC-32 of a block of data all at once
///
/// @param ulCount Number of bytes in the data block
/// @param char Data block
/// @return uint32_t Result
///
uint32_t CalculateBlockCRC32(uint32_t ulCount, unsigned char* ucBuffer)
{
    uint32_t ulTemp1;
    uint32_t ulTemp2;
    uint32_t ulCRC = 0;
    while (ulCount-- != 0) {
        ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
        ulTemp2 = CRC32Value(((uint32_t)ulCRC ^ *ucBuffer++) & 0xff);
        ulCRC = ulTemp1 ^ ulTemp2;
    }
    return (ulCRC);
}

}  // namespace driver
}  // namespace device
}  // namespace hera
}  // namespace wayz