///
/// @file device_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class DeviceData and class SensorData
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///


#include "device_data.hpp"

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

DisplayDataPtr DisplayData::broken_data()
{

    auto* disp_data = new DisplayData();
    disp_data->is_valid = false;
    disp_data->is_jpeg = false;
    disp_data->sequence = 0;
    disp_data->timestamp_intrinsic_ns = time::Timestamp::now();
    disp_data->data = "";
    return DisplayDataPtr(disp_data);
}

DisplayDataPtr DisplayData::create_from(std::vector<SensorDataPtr>&& sensor_datas)
{
    if (sensor_datas.size() == 0) {
        log::warn << "DisplayParser: Sensor data size is 0" << log::endl;
        return broken_data();
    }

    auto disp_data = DisplayDataPtr(new DisplayData());
    disp_data->is_valid = true;
    disp_data->sequence = sensor_datas[0]->sequence;
    disp_data->timestamp_intrinsic_ns = sensor_datas[0]->timestamp_intrinsic_ns;

    switch (sensor_datas[0]->sensor_data_type) {
    case SensorDataType::Dummy:
        disp_data->data = std::move(parse<SensorDataType::Dummy>(std::move(sensor_datas), disp_data->is_jpeg));
        break;
    case SensorDataType::DummyImage:
        disp_data->data = std::move(parse<SensorDataType::DummyImage>(std::move(sensor_datas), disp_data->is_jpeg));
        break;
    case SensorDataType::ImuMagneticField:
        disp_data->data =
                std::move(parse<SensorDataType::ImuMagneticField>(std::move(sensor_datas), disp_data->is_jpeg));
        break;
    case SensorDataType::PointsXYZI:
        disp_data->data = std::move(parse<SensorDataType::PointsXYZI>(std::move(sensor_datas), disp_data->is_jpeg));
        break;
    case SensorDataType::CompressedImage:
        disp_data->data =
                std::move(parse<SensorDataType::CompressedImage>(std::move(sensor_datas), disp_data->is_jpeg));
        break;
    case SensorDataType::NavSatFix:
        disp_data->data = std::move(parse<SensorDataType::NavSatFix>(std::move(sensor_datas), disp_data->is_jpeg));
        break;
    case SensorDataType::InsBestPosition:
        disp_data->data =
                std::move(parse<SensorDataType::InsBestPosition>(std::move(sensor_datas), disp_data->is_jpeg));
        break;
    case SensorDataType::OdometryOrientation:
        disp_data->data =
                std::move(parse<SensorDataType::OdometryOrientation>(std::move(sensor_datas), disp_data->is_jpeg));
        break;
    default:
        log::error << "DisplayParser:: Invalid Sensor Data Type" << log::endl;
        return broken_data();
        break;
    }
    return disp_data;
}

}  // namespace data
}  // namespace device
}  // namespace hera
}  // namespace wayz