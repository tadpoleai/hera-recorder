///
/// @file result.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of function to get result from hera-slam-bridge
/// @version 0.1
/// @date 2020-02-08
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "result.hpp"

namespace wayz {
namespace hera {
namespace slam {

std::unique_ptr<IPCResult> Result::handler(const ipc::OpenMode mode)
{
    auto ipc_result = IPCResult::create();
    ipc_result->open(IPCResultKey, mode);
    return ipc_result;
}

ResultPtr Result::deserialize(void* src, size_t max_size)
{
    uint32_t length = *(uint32_t*)(src);
    if (length > max_size) {
        return nullptr;
    }

    auto result = std::make_shared<Result>();
    memcpy(&result->height, (char*)(src) + 4, 8);
    result->data.resize(length - 4 - 8);
    memcpy(result->data.data(), (char*)(src) + 4 + 8, length - 4 - 8);

    return result;
}

size_t Result::serialize(void* dest, size_t max_size) const
{
    uint32_t length = 4 + 8 + data.size();
    if (length > max_size) {
        return 0;
    }

    memcpy(dest, &length, 4);
    memcpy((char*)dest + 4, (const void*)&height, 8);
    memcpy((char*)dest + 4 + 8, data.data(), data.size());

    return length;
}

}  // namespace slam
}  // namespace hera
}  // namespace wayz