///
/// @file result.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of function to get result from hera-slam-bridge
/// @version 0.1
/// @date 2020-02-08
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <memory>

#include "common/ipc/ipc_queue.hpp"
#include "common/utils/time.hpp"

namespace wayz {
namespace hera {
namespace slam {

class Result;
static constexpr auto IPCResultNumElement = 2;              ///< Single buffer
static constexpr auto IPCResultElementSize = 32 * 1 << 20;  ///< Max size of result, 32MiB
using IPCResult = ipc::IPCQueue<Result>;
using ResultPtr = std::shared_ptr<Result>;

#pragma pack(push, 1)

///
/// @brief Result of slam
///
class Result {
private:
    static constexpr auto IPCResultKey = 1;  ///< Key of IPC

public:
    Result() = default;
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

public:
    ///
    /// @brief Get handler of IPC Result
    ///
    /// @param mode ipc::OpenMode mode of ipc (write/read)
    /// @return std::unique_ptr<IPCResult> handler
    ///
    static std::unique_ptr<IPCResult> handler(const ipc::OpenMode mode = ipc::OpenMode::Read);

public:
    size_t serialize(void* dest, size_t max_size) const;       ///< for IPC
    static ResultPtr deserialize(void* src, size_t max_size);  ///< for IPC

public:
    int32_t height;             ///< Height of jpeg data
    int32_t width;              ///< Width of jpeg data
    std::vector<uint8_t> data;  ///< Jpeg raw data
};

#pragma pack(pop)

}  // namespace slam
}  // namespace hera
}  // namespace wayz