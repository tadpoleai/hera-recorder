///
/// @file ViusalPoseWithCovariance.hpp
/// @author frye.zhang (frye.zhang@wayz.ai)
/// @brief Header of viusal localization result
/// @version 0.1
/// @date 2020-04-23
///
/// @copyright Copyright 2018-2025 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <memory>

#include "hera/common/ipc/ipc_queue.hpp"

namespace wayz {
namespace vslam {

#pragma pack(push, 1)
///
/// @brief Pose with covariance of visual localization result
///
class ViusalPoseWithCovariance {

private:
    static constexpr auto IPCVisualPoseKey = 0x200;  ///< Key of IPC
    static constexpr auto IPCVisualPoseNumElement = 16;
    static constexpr auto IPCVisualPoseElementSize = 512;

public:
    using IPCVisualPose = hera::ipc::IPCQueue<ViusalPoseWithCovariance>;
    using ViusalPosePtr = std::shared_ptr<ViusalPoseWithCovariance>;

public:
    ///
    /// @brief Get handler of IPC
    ///
    /// @param mode ipc::OpenMode mode of ipc (write/read)
    /// @return std::unique_ptr<IPCVisualPose> handler
    ///
    static std::unique_ptr<IPCVisualPose> handler(const hera::ipc::OpenMode mode = hera::ipc::OpenMode::Read)
    {
        auto ipc_result = IPCVisualPose::create();
        ipc_result->open(IPCVisualPoseKey, mode, false, IPCVisualPoseNumElement, IPCVisualPoseElementSize);
        return ipc_result;
    }

public:
    size_t serialize(void* dest, size_t max_size)  ///< for IPC
    {
        if (max_size < sizeof(ViusalPoseWithCovariance)) {
            return 0;
        }

        memcpy(dest, this, sizeof(ViusalPoseWithCovariance));
        return sizeof(ViusalPoseWithCovariance);
    }

    static ViusalPosePtr deserialize(void* src, size_t max_size)  ///< for IPC
    {
        if (max_size < sizeof(ViusalPoseWithCovariance)) {
            return nullptr;
        }

        auto result = std::make_shared<ViusalPoseWithCovariance>();
        memcpy(result.get(), src, sizeof(ViusalPoseWithCovariance));

        return result;
    }

public:
    int64_t time_stamp;
    std::array<double, 3> translation{0.0};
    std::array<double, 4> orientation{0.0};
    std::array<double, 36> covariance{0.0};  ///< T0 T1 T2 R0 R1 R2
};

#pragma pack(pop)

}  // namespace vslam
}  // namespace wayz
