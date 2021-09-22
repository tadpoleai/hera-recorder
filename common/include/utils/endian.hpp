///
/// @file endian.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @version 0.1
/// @date 2021-09-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <stdint.h>

namespace wayz {
namespace hera {
namespace common {

namespace endian_impl {
void reverse_endian_2(const uint8_t* input, uint8_t* output)
{
    output[1] = input[0];
    output[0] = input[1];
}

void reverse_endian_4(const uint8_t* input, uint8_t* output)
{
    output[3] = input[0];
    output[2] = input[1];
    output[1] = input[2];
    output[0] = input[3];
}

void reverse_endian_8(const uint8_t* input, uint8_t* output)
{
    output[7] = input[0];
    output[6] = input[1];
    output[5] = input[2];
    output[4] = input[3];
    output[3] = input[4];
    output[2] = input[5];
    output[1] = input[6];
    output[0] = input[7];
}
}  // namespace endian_impl

uint16_t reverse_endian(const uint16_t& input)
{
    uint16_t output;
    endian_impl::reverse_endian_2((uint8_t*)(void*)&input, (uint8_t*)(void*)&output);
    return output;
}

int16_t reverse_endian(const int16_t& input)
{
    int16_t output;
    endian_impl::reverse_endian_2((uint8_t*)(void*)&input, (uint8_t*)(void*)&output);
    return output;
}

uint32_t reverse_endian(const uint32_t& input)
{
    uint32_t output;
    endian_impl::reverse_endian_4((uint8_t*)(void*)&input, (uint8_t*)(void*)&output);
    return output;
}

int32_t reverse_endian(const int32_t& input)
{
    int32_t output;
    endian_impl::reverse_endian_4((uint8_t*)(void*)&input, (uint8_t*)(void*)&output);
    return output;
}

float reverse_endian(const float& input)
{
    float output;
    endian_impl::reverse_endian_4((uint8_t*)(void*)&input, (uint8_t*)(void*)&output);
    return output;
}

uint64_t reverse_endian(const uint64_t& input)
{
    uint64_t output;
    endian_impl::reverse_endian_8((uint8_t*)(void*)&input, (uint8_t*)(void*)&output);
    return output;
}

int64_t reverse_endian(const int64_t& input)
{
    int64_t output;
    endian_impl::reverse_endian_8((uint8_t*)(void*)&input, (uint8_t*)(void*)&output);
    return output;
}

double reverse_endian(const double& input)
{
    double output;
    endian_impl::reverse_endian_8((uint8_t*)(void*)&input, (uint8_t*)(void*)&output);
    return output;
}

}  // namespace common
}  // namespace hera
}  // namespace wayz