///
/// @file device_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class DeviceData
/// @version 0.1
/// @date 2019-11-06
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

class DeviceData;

///
/// @brief Shared pointer to DeviceData
///
using DeviceDataPtr = std::shared_ptr<DeviceData>;

///
/// @brief Abstract base class device data
///
/// Provides common interfaces and realizes common operations
/// @note This class should be declared as pack(1)
/// @see DeviceDataType
class DeviceData {
    ///
    /// @brief Allow SensorData::create_from()
    /// @see SensorData
    ///
    friend class SensorData;

public:
    DeviceData() = delete;

public:
    ///
    /// @brief Create a storage
    ///
    /// @param length memory size to allocate, in bytes
    /// @return DeviceDataPtr shared pointer to DeviceData
    ///
    static DeviceDataPtr create(uint32_t length);

    ///
    /// @brief Overload of create()
    ///
    /// @param length memory size to allocate, in bytes
    /// @param id device id
    /// @param type device vendor type enum
    /// @param msgtype device data type enum
    /// @param sequence sequece
    /// @return DeviceDataPtr shared pointer to DeviceData
    /// @see create()
    ///
    static DeviceDataPtr create(uint32_t length,
                                uint32_t id,
                                DeviceVendorType type,
                                DeviceDataType msgtype,
                                uint32_t sequence);

    ///
    /// @brief Read a device data from ifstream
    ///
    /// @param ifs ifstream to read from
    /// @return DeviceDataPtr shared pointer to DeviceData
    /// @return DeviceDataPtr nullptr when either data read is invalid or ifs is closed/empty/ended
    static DeviceDataPtr read_from(std::ifstream& ifs);

    ///
    /// @brief Write a device data to storage
    ///
    /// @param ofs ofstream to write to
    /// @return size_t size accurately written to ofs
    /// @note this function blocks during writing, called in Storage's writing thread
    /// @see Storage
    size_t write_to(std::ofstream& ofs) const;

    ///
    /// @brief Get the receive timestamp receive
    ///
    /// @return uint64_t receive timestamp
    inline uint64_t get_timestamp_receive_ns() const noexcept
    {
        return timestamp_receive_ns;
    }

    ///
    /// @brief Get the total length of DeviceData
    ///
    /// @return uint64_t total length, in bytes
    inline uint64_t get_length() const noexcept
    {
        return length;
    }

    ///
    /// @brief Get the device id of DeviceData
    ///
    /// @return uint32_t device id
    inline uint32_t get_device_id() const noexcept
    {
        return device_id;
    }

    ///
    /// @brief Check if device data type is specific type
    ///
    /// @param type device data type to compare with
    /// @return true type is the same
    /// @return false type is not the same
    inline bool is_type(DeviceDataType type) const noexcept
    {
        return message_type == type;
    }

    ///
    /// @brief Get member vendor type
    ///
    /// @return device_vendor_type
    ///
    inline auto get_vendor_type() const noexcept
    {
        return device_vendor_type;
    }

private:
    uint32_t length;                      ///< Total length, in bytes
    uint32_t device_id;                   ///< ID of device
    DeviceVendorType device_vendor_type;  ///< Device vendor type
    DeviceDataType message_type;          ///< Device data type
    uint32_t sequence;                    ///< Sequence, increased by 1 every time
    uint64_t timestamp_receive_ns;        ///< Timstamp of data received, UTC, in ns
};

#pragma pack(pop)

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz