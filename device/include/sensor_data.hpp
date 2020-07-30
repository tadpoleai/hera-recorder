///
/// @file sensor_data.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Sensor Data
/// @date 2020-06-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "types.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

#pragma pack(push, 1)

class SensorData;
class DeviceData;

using SensorDataPtr = std::shared_ptr<SensorData>;

///
///
/// @brief Abstract base class sensor data
///
/// Provides common interfaces and realizes common operations
///
/// @see SensorDataType
class SensorData {
public:
    using DeviceDataPtr = std::shared_ptr<DeviceData>;

public:
    SensorData() = delete;

    ///
    /// @brief Create a SensorData from DeviceData
    ///
    /// @param storage_data device data to refer from
    /// @param type sensor data type
    /// @param length memory size to allocate, in bytes
    /// @return SensorDataPtr shared pointer to SensorDataPtr
    ///
    static SensorDataPtr create_from(const DeviceDataPtr& storage_data,
                                     const SensorDataType type,
                                     const uint32_t length);

    ///
    /// @brief Create a SensorData from DeviceData
    ///
    /// @param type sensor data type
    /// @param length memory size to allocate, in bytes
    /// @param id device id
    /// @param sequence sequence of sensor data
    /// @return SensorDataPtr shared pointer to SensorDataPtr
    ///
    static SensorDataPtr create_direct(const SensorDataType type,
                                       const uint32_t length,
                                       const uint32_t id,
                                       const uint32_t sequence);

    ///
    /// @brief Create a broken sensor_data
    ///
    /// @return SensorDataPtr shared pointer to a broken sensor_data
    ///
    static SensorDataPtr broken_data();

    ///
    /// @brief Create an end of file sensor_data
    ///
    /// @return SensorDataPtr shared pointer to an end of file sensor_data
    ///
    static SensorDataPtr end_of_file();

    ///
    /// @brief Serialize to memory
    ///
    /// @param dest destination pointer
    /// @param max_size max size of destination
    /// @return size_t 0 if failed, otherwise size after serialization
    ///
    /// @see ipc::IPCQueue
    size_t serialize(void* dest, size_t max_size) const;

    ///
    /// @brief Deserialize from memory
    ///
    /// @param src source pointer
    /// @param max_size max size of source
    /// @return SensorDataPtr a shared pointer to Deserialized SensorData, if succeed, otherwise nullptr
    ///
    /// @see ipc::IPCQueue
    static SensorDataPtr deserialize(void* src, size_t max_size);

public:
    uint32_t length;                  ///< Total length, in bytes
    uint32_t sensor_id;               ///< ID of sensor
    SensorDataType sensor_data_type;  ///< Sensor data type
    uint32_t sequence;                ///< Sequence, copied from DeviceData::sequence
    uint64_t timestamp_intrinsic_ns;  ///< Timstamp of device intrinsic, i.e. with synchronization,
                                      /// UTC, in ns
    uint8_t data[0];                  ///< Start address of derived data
};

#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz