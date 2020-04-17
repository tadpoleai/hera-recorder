///
/// @file device_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class DeviceData and class SensorData
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

#include "device_types.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

#pragma pack(push, 1)

class DeviceData;
class SensorData;
class DisplayData;

///
/// @brief Shared pointer to DeviceData
///
using DeviceDataPtr = std::shared_ptr<DeviceData>;

///
/// @brief Shared pointer to SensorData
///
using SensorDataPtr = std::shared_ptr<SensorData>;

///
/// @brief Shared pointer to DisplayData
///
using DisplayDataPtr = std::shared_ptr<DisplayData>;

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

///
/// @brief Abstract base class sensor data
///
/// Provides common interfaces and realizes common operations
///
/// @see SensorDataType
class SensorData {
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

///
/// @brief Final class display data
///
/// Which contains string or jpeg data to display for viewer
///
class DisplayData final {
public:
    ///
    /// @brief Create a broken DisplayData
    ///
    static DisplayDataPtr broken_data();

    ///
    /// @brief Create a DisplayData from SensorData
    ///
    /// @param sensor_datas vector of SensorData
    /// @param data Converted data (compressed jpeg or parsed human-readable string)
    /// @return DisplayDataPtr shared pointer to DisplayData
    ///
    /// @note This function calls parse()
    ///
    static DisplayDataPtr create_from(std::vector<SensorDataPtr>&& sensor_datas);

private:
    DisplayData() = default;

    ///
    /// @brief Convert a SensorData to DisplayData
    ///
    /// @tparam T SensorDataType sensor data type
    /// @param sensor_datas vector of SensorData
    /// @param is_jpeg [out] is data jpeg or string
    /// @see folder: <category>/<category>_data.cpp
    template<SensorDataType T>
    static std::string parse(std::vector<SensorDataPtr>&& sensor_datas, bool& is_jpeg);

public:
    bool is_valid;                    ///< Is data valid
    bool is_jpeg;                     ///< Is jpeg or string
    uint32_t sequence;                ///< Sequence, copied from SensorData::sequence
    uint64_t timestamp_intrinsic_ns;  ///< Timstamp, copied from SensorData::sequence
    std::string data;                 ///< Data in string or jpeg binary
};

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz