///
/// @file bridge.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of Node to bridge between ROS and Hera
/// @version 0.2
/// @date 2020-02-04
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <hera/common/ipc/ipc_queue.hpp>
#include <hera/device/include.hpp>

using namespace wayz::hera;

namespace wayz {

///
/// @brief Bridge data / messages between Fusion_Location_SDK and Hera
///
class Bridge {
public:
    ///
    /// @brief Construct a new Bridge object
    ///
    Bridge();

    ///
    /// @brief Destroy the Bridge objectÎ
    ///
    ~Bridge();

    ///
    /// @brief Block and wait for message
    ///
    void spin();

private:
    ///
    /// @brief Handler for lidar data from hera
    ///
    /// @param data lidar data
    ///
    void camera_handler(const device::data::Image* const data);


private:
    std::unique_ptr<ipc::IPCQueue<device::data::SensorData>> ipc_queue_;  ///< ipc queue interface from hera

    /// Hera goes Fusion_Location_SDK Parameters
    int camera_sensor_id_;  ///< Restrict sensor id for camera, only accept id-matched data, -1 to disable
};

}  // namespace wayz