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
#include <fstream>
#include <memory>
#include <string>

#include "common/utils/timestamp.hpp"
#include "device_types.hpp"

namespace wayz {
namespace hera {

#pragma pack(push, 1)

class StorageData;
class SensorData;
///
/// @brief Shared pointer to StorageData
///
using StorageDataPtr = std::shared_ptr<StorageData>;

///
/// @brief Shared pointer to SensorData
///
using SensorDataPtr = std::shared_ptr<SensorData>;

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
    /// Allocate memory sized length, managed by a shares pointer,
    /// to avoid possible memory leak
    /// @note Length should be assigned by derived classes' implementation.
    /// @note Usually, for an derived class without variable-length buf,
    /// length should be sizeof(SomeInheritedStorageData).
    /// @note For an derived class with variable-length buf,
    /// length should be sizeof(SomeInheritedStorageData) added by
    /// size of [variable-length buf]
    static inline auto create(uint32_t length)
    {
        auto* data = reinterpret_cast<StorageData*>(new uint8_t[length]);
        data->length = length;
        return StorageDataPtr(data, [](void* ptr) { delete[](uint8_t*)(ptr); });
    }

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
    /// Fill device vendor type, storage data type and sequece by arguments
    /// Fill receive timestamp automatically
    /// @note timestamp will be automatically filled
    static inline auto create(uint32_t length,
                              DeviceVendorType type,
                              StorageDataType msgtype,
                              uint32_t sequence)
    {
        auto data = create(length);
        data->device_type = type;
        data->message_type = msgtype;
        data->sequence = sequence;
        data->timestamp_receive_ns = Timestamp::now();
        return data;
    }

    ///
    /// @brief Read a storage data from ifstream
    ///
    /// @param ifs ifstream to read from
    /// @return StorageDataPtr shared pointer to StorageData
    /// @return StorageDataPtr nullptr when either data read is invalid or ifs is closed/empty/ended
    static inline StorageDataPtr read_from(std::ifstream& ifs)
    {
        try {
            uint32_t length;
            ifs.read((char*)&length, sizeof(length));
            if (ifs.gcount() != sizeof(length)) {
                return nullptr;
            }

            auto data = create(length);
            ifs.read((char*)data.get() + sizeof(length), length - sizeof(length));
            if (ifs.gcount() != uint32_t(length - sizeof(length))) {
                return nullptr;
            }
            return data;
        } catch (...) {
            return nullptr;
        }
    }

    ///
    /// @brief Write a storage data to storage
    ///
    /// @param ofs ofstream to write to
    /// @return size_t size accurately written to ofs
    /// @note this function blocks during writing, called in Storage's writing thread
    /// @see Storage
    inline size_t write_to(std::ofstream& ofs) const
    {
        try {
            ofs.write((const char*)this, length);
            return length;
        } catch (...) {
            return 0;
        }
    }

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
    /// Allocate memory sized length, managed by a shares pointer,
    /// copy sequence from storage_data
    ///
    /// @note Length should be assigned derived classes' implementation.
    /// @note Usually, for an derived class without variable-length buf,
    /// length should be sizeof(SomeInheritedSensorData).
    /// @note For an derived class with variable-length buf,
    /// length should be sizeof(SomeInheritedSensorData) added by
    /// size of [variable-length buf]
    /// @note This function is called by Device::convert()
    /// @see StorageData
    /// @see Device::convert()
    static inline auto create_from(const StorageDataPtr& storage_data,
                                   SensorDataType type,
                                   uint32_t length)
    {
        auto* data = reinterpret_cast<SensorData*>(new uint8_t[length]);
        data->length = length;
        data->sensor_data_type = type;
        data->sequence = storage_data->sequence;
        return SensorDataPtr(data, [](void* ptr) { delete[](uint8_t*)(ptr); });
    }

    ///
    /// @brief Create a broken sensor_data
    ///
    /// @return SensorDataPtr shared pointer to a broken sensor_data
    /// @note This function is called by Device::convert()
    /// @see Device::convert()
    static inline SensorDataPtr broken_data()
    {
        auto length = sizeof(SensorData);
        auto* data = reinterpret_cast<SensorData*>(new uint8_t[length]);
        data->length = length;
        data->sensor_data_type = SensorDataType::Broken;
        data->sequence = 0;
        return SensorDataPtr(data, [](void* ptr) { delete[](uint8_t*)(ptr); });
    }

public:
    uint32_t length;                  ///< Total length, in bytes
    SensorDataType sensor_data_type;  ///< Sensor data type
    uint32_t sequence;                ///< Sequence, copied from StorageData::sequence
    uint64_t timestamp_intrinsic_ns;  ///< Timstamp of device intrinsic, i.e. with synchronization,
                                      /// UTC, in ns
};

#pragma pack(pop)

}  // namespace hera
}  // namespace wayz