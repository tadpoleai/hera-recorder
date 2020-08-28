///
/// @file device_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class DeviceData and class SensorData
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///


#include "storage.hpp"

#include <fstream>

#include "common/include/logger/logger.hpp"
#include "common/include/utils/time.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace data {

///
/// Allocate memory sized length, managed by a shares pointer,
/// to avoid possible memory leak
/// @note Length should be assigned by derived classes' implementation.
/// @note Usually, for an derived class without variable-length buf,
/// length should be sizeof(SomeInheritedDeviceData).
/// @note For an derived class with variable-length buf,
/// length should be sizeof(SomeInheritedDeviceData) added by
/// size of [variable-length buf]
///
DeviceDataPtr DeviceData::create(uint32_t length)
{
    auto* data = reinterpret_cast<DeviceData*>(new uint8_t[length]);
    data->length = length;
    return DeviceDataPtr(data, [](void* ptr) { delete[](uint8_t*)(ptr); });
}

///
/// Fill device vendor type, device data type and sequece by arguments
/// Fill receive timestamp automatically
/// @note timestamp will be automatically filled
///
DeviceDataPtr DeviceData::create(uint32_t length,
                                 uint32_t id,
                                 DeviceVendorType vendor_type,
                                 DeviceDataType msgtype,
                                 uint32_t sequence)
{
    auto data = create(length);
    data->device_id = id;
    data->device_vendor_type = vendor_type;
    data->message_type = msgtype;
    data->sequence = sequence;
    data->timestamp_receive_ns = time::Timestamp::now();
    return data;
}

DeviceDataPtr DeviceData::read_from(std::ifstream& ifs)
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

size_t DeviceData::write_to(std::ofstream& ofs) const
{
    try {
        ofs.write((const char*)this, length);
        return length;
    } catch (...) {
        return 0;
    }
}

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz