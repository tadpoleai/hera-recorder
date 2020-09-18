///
/// @file android.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-07-29
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cmath>

#include "android_data.hpp"
#include "device.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace external {
namespace android {

///
/// @brief Android recorded data parser, no driver
///
class Android final : public Device {
public:
    ///
    /// @brief Construct a new Aceinna object
    /// @note never used
    Android(const uint32_t id,
            const std::string& vendor_type,
            const std::string& name,
            const bool forward,
            ipc::IPCQueue<data::SensorData>* const ipc_queue,
            storage::StorageManager* const storage) :
        Device(id, vendor_type, name, forward, ipc_queue, storage, HistoryDepth_, EssentialParameterTypes)
    {}
    Android(const Android&) = delete;
    Android& operator=(const Android&) = delete;

    ///
    /// @brief Destroy the Aceinna object
    ///
    /// calls Device::stop()
    virtual ~Android()
    {
        stop();
    }

    virtual data::SensorDataPtr convert(data::DeviceDataPtr& storage_data) override
    {
        return do_convert(storage_data);
    }

    ///
    /// @brief Static convert function for read / convert / replay
    ///
    static data::SensorDataPtr do_convert(data::DeviceDataPtr& storage_data);

public:
    static const std::vector<std::string> EssentialParameterTypes;  ///< Essential Parameters for device

    static const std::vector<std::string> OptionalParameterTypes;  ///< Optional Parameters for device

private:
    static constexpr size_t HistoryDepth_ = 1;  ///< History Depth, 1
};

}  // namespace android
}  // namespace external
}  // namespace device
}  // namespace hera
}  // namespace wayz