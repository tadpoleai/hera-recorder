/// 
/// @file plugin_entry.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief 
/// @date 2020-08-19
/// 
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
/// 

#include <chrono>
#include <cmath>
#include <cstdlib>

#include "plugin_common.hpp"
#include "plugin_data.hpp"
#include "plugin_param.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace dummy {
namespace image {

///
/// @brief A dummy device image, for sample, Derived from Device
///
HERA_PLUGIN_DEFINE_START(1)

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS
#endif

HERA_PLUGIN_DEFINE_END

HERA_PLUGIN_EXPORT(DummyImage, "dummy/image")

#ifdef WITH_DRIVER

/// Parse parameters first
/// @note For parameters delcared in essential parameters,
/// just parse it.
/// @note For parameters not delcared in essential parameters,
/// just existance first, then parse it
/// @note For non-string value, use stoi, stol, stod inside a try-catch block
///
/// Then allocate resource and connect
HeraErrno DevicePlugin::connect()
{
    // Open some resources and handlers
    // ...

    return HeraErrno::Success;
}

/// Free resources and handlers opened by connect()
///
void DevicePlugin::disconnect() {}


/// See source for a code sample of,
/// how to create and return a valid data
data::DeviceDataPtr DevicePlugin::fetch()
{
    // Image::fetch() blocks when fetching for a certain period
    std::this_thread::sleep_for(std::chrono::microseconds(local_parameters_.get_PeriodUs()));

    // Some devices randomly fails during fetching,
    // in case of that, return a nullptr
    if (std::rand() > RAND_MAX * 0.9) {
        return nullptr;
    }

    // Total length of device data
    auto length = sizeof(ImageData);
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::DummyImage,
                                         DeviceDataType::DummyImageData,
                                         sequence_++);
    auto derived_data = static_cast<ImageData*>(data.get());

    derived_data->data.image_width = 480;
    derived_data->data.image_height = 480;

    return data;
}

/// See source for a code sample of,
/// how to implement this function
/// @note For a mutable parameter, apply the new value
/// @note For an immutable, return HeraErrno::ImmutableParameter
/// @note Return HeraErrno::UnimplementedParameter by default
HeraErrno DevicePlugin::adjust_parameter(const std::string& type, const std::string& value)
{
    return HeraErrno::OK;
}
#endif

/// See source for a code sample of,
/// how to create and return a valid converted data
/// @note Check the type of device data first,
data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameters)
{
    if (!storage_data->is_type(DeviceDataType::DummyImageData)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<ImageData*>(storage_data.get());

    // Create a SensorData from DeviceData
    uint32_t length = sizeof(data::DummyImage);
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::DummyImage, length);
    auto dummy_sensor_data = static_cast<data::DummyImage*>(sensor_data.get());

    // Parse Data
    dummy_sensor_data->timestamp_intrinsic_ns = raw_data->get_timestamp_receive_ns();
    dummy_sensor_data->image_width = raw_data->data.image_width;
    dummy_sensor_data->image_height = raw_data->data.image_height;

    return sensor_data;
}

}  // namespace image
}  // namespace dummy
}  // namespace device
}  // namespace hera
}  // namespace wayz