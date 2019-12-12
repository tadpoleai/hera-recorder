///
/// @file device_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class StorageData and class SensorData
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

#pragma pack(push, 1)

class StorageData;
class SensorData;
class DisplayData;

///
/// @brief Shared pointer to StorageData
///
using StorageDataPtr = std::shared_ptr<StorageData>;

///
/// @brief Shared pointer to SensorData
///
using SensorDataPtr = std::shared_ptr<SensorData>;

///
/// @brief Shared pointer to DisplayData
///
using DisplayDataPtr = std::shared_ptr<DisplayData>;

///
/// @brief Abstract base class storage data
///
/// Provides common interfaces and realizes common operations
/// @note This class should be declared as pack(1)
/// @see StorageDataType
class StorageData {
    ///
    /// @brief Allow SensorData::create_from()
    /// @see SensorData
    ///
    friend class SensorData;

public:
    StorageData() = delete;

public:
    ///
    /// @brief Create a storage
    ///
    /// @param length memory size to allocate, in bytes
    /// @return StorageDataPtr shared pointer to StorageData
    ///
    static StorageDataPtr create(uint32_t length);

    ///
    /// @brief Overload of create()
    ///
    /// @param length memory size to allocate, in bytes
    /// @param type device vendor type enum
    /// @param msgtype storage data type enum
    /// @param sequence sequece
    /// @return StorageDataPtr shared pointer to StorageData
    /// @see create()
    ///
    static StorageDataPtr create(uint32_t length, DeviceVendorType type, StorageDataType msgtype, uint32_t sequence);

    ///
    /// @brief Read a storage data from ifstream
    ///
    /// @param ifs ifstream to read from
    /// @return StorageDataPtr shared pointer to StorageData
    /// @return StorageDataPtr nullptr when either data read is invalid or ifs is closed/empty/ended
    static StorageDataPtr read_from(std::ifstream& ifs);

    ///
    /// @brief Write a storage data to storage
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
    /// @brief Get the total length of StorageData
    ///
    /// @return uint64_t total length, in bytes
    inline uint64_t get_length() const noexcept
    {
        return length;
    }

    ///
    /// @brief Check if storage data type is specific type
    ///
    /// @param type storage data type to compare with
    /// @return true type is the same
    /// @return false type is not the same
    inline bool is_type(StorageDataType type) const noexcept
    {
        return message_type == type;
    }

private:
    uint32_t length;                ///< Total length, in bytes
    DeviceVendorType device_type;   ///< Device vendor type
    StorageDataType message_type;   ///< Storage data type
    uint32_t sequence;              ///< Sequence, increased by 1 every time
    uint64_t timestamp_receive_ns;  ///< Timstamp of data received, UTC, in ns
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
    /// @brief Create a SensorData from StorageData
    ///
    /// @param storage_data storage data to refer from
    /// @param type sensor data type
    /// @param length memory size to allocate, in bytes
    /// @return SensorDataPtr shared pointer to SensorDataPtr
    ///
    static SensorDataPtr create_from(const StorageDataPtr& storage_data, SensorDataType type, uint32_t length);

    ///
    /// @brief Create a broken sensor_data
    ///
    /// @return SensorDataPtr shared pointer to a broken sensor_data
    ///
    static SensorDataPtr broken_data();

public:
    uint32_t length;                  ///< Total length, in bytes
    SensorDataType sensor_data_type;  ///< Sensor data type
    uint32_t sequence;                ///< Sequence, copied from StorageData::sequence
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


}  // namespace hera
}  // namespace wayz