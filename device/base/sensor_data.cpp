///
/// @file device_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class SensorData
/// @version 0.1
/// @date 2020-06-18
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///


#include "sensor_data.hpp"

#include <fstream>
#include <string.h>

#include "common/include/logger/logger.hpp"
#include "common/include/utils/time.hpp"
#include "device_data.hpp"
#include "sensor_data_types.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

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
/// @see DeviceData
/// @see Device::convert()
SensorDataPtr SensorData::create_from(const DeviceDataPtr& storage_data,
                                      const SensorDataType type,
                                      const uint32_t length)
{
    if (!storage_data) {
        return broken_data();
    }
    auto* data = reinterpret_cast<SensorData*>(new uint8_t[length]);
    data->length = length;
    data->sensor_id = storage_data->device_id;
    data->sensor_data_type = type;
    data->sequence = storage_data->sequence;
    return SensorDataPtr(data, [](void* ptr) { delete[](uint8_t*)(ptr); });
}

///
/// Allocate memory sized length, managed by a shares pointer,
///
/// @note Length should be assigned derived classes' implementation.
/// @note Usually, for an derived class without variable-length buf,
/// length should be sizeof(SomeInheritedSensorData).
/// @note For an derived class with variable-length buf,
/// length should be sizeof(SomeInheritedSensorData) added by
/// size of [variable-length buf]
/// @see DeviceData
/// @see Device::convert()
SensorDataPtr SensorData::create_direct(const SensorDataType type,
                                        const uint32_t length,
                                        const uint32_t id,
                                        const uint32_t sequence)
{
    auto* data = reinterpret_cast<SensorData*>(new uint8_t[length]);
    data->length = length;
    data->sensor_id = id;
    data->sensor_data_type = type;
    data->sequence = sequence;
    return SensorDataPtr(data, [](void* ptr) { delete[](uint8_t*)(ptr); });
}

///
/// @note This function is called by Device::convert()
/// @see Device::convert()
SensorDataPtr SensorData::broken_data()
{
    auto length = sizeof(SensorData);
    auto* data = reinterpret_cast<SensorData*>(new uint8_t[length]);
    data->length = length;
    data->sensor_data_type = SensorDataType::Broken;
    data->sequence = 0;
    return SensorDataPtr(data, [](void* ptr) { delete[](uint8_t*)(ptr); });
}


SensorDataPtr SensorData::end_of_file()
{
    auto length = sizeof(SensorData);
    auto* data = reinterpret_cast<SensorData*>(new uint8_t[length]);
    data->length = length;
    data->sensor_data_type = SensorDataType::EndOfFile;
    data->sequence = 0;
    return SensorDataPtr(data, [](void* ptr) { delete[](uint8_t*)(ptr); });
}

/// Check max_size and then copy to destination
///
size_t SensorData::serialize(void* dest, size_t max_size) const
{
    if (length > max_size) {
        log::debug << "SensorData: Serialize Max Size Over" << log::endl;
        return 0;
    }
    memcpy(dest, this, length);
    return length;
}

/// Read length and then new a data and copy from source
///
SensorDataPtr SensorData::deserialize(void* src, size_t max_size)
{
    uint32_t length = *(uint32_t*)(src);
    if (length > max_size) {
        return nullptr;
    }
    auto* data = reinterpret_cast<SensorData*>(new uint8_t[length]);
    memcpy(data, src, length);
    return SensorDataPtr(data, [](void* ptr) { delete[](uint8_t*)(ptr); });
}

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz