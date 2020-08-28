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
namespace foobar {

///
/// @brief A dummy device foobar, for sample, Derived from Device
///
HERA_PLUGIN_DEFINE_START(5)

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS
#endif

HERA_PLUGIN_DEFINE_END

HERA_PLUGIN_EXPORT(DummyFoobar, "dummy/foobar")

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
    // Foobar::fetch() blocks when fetching for a certain period
    std::this_thread::sleep_for(std::chrono::microseconds(local_parameters_.get_PeriodUs()));

    // Some devices randomly fails during fetching,
    // in case of that, return a nullptr
    if (std::rand() > RAND_MAX * 0.9) {
        return nullptr;
    }

    // Length of variable-lengthed-buf fields, in bytes
    std::string string_message = local_parameters_.get_Message();
    auto string_length = string_message.size();
    // Total length of device data
    auto length = sizeof(FoobarData) + string_length;
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::DummyFoobar,
                                         DeviceDataType::DummyFoobarData,
                                         sequence_++);
    auto derived_data = static_cast<FoobarData*>(data.get());

    // Set non-buf-typed fields
    derived_data->data.int_value = local_parameters_.get_IntValue();
    for (auto i = 0; i < 4; ++i) {
        derived_data->data.char_value[i] = local_parameters_.get_IntValue() % 0x100;
    }

    // Set length of variable-lengthed-buf fields' length. in bytes
    derived_data->data.string_message_length = string_length;
    // Use Memcpy to buf-typed fields
    memcpy(derived_data->data.string_message, string_message.data(), string_length);

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
                                             const ParametersInterface* parameter)
{
    if (!storage_data->is_type(DeviceDataType::DummyFoobarData)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<FoobarData*>(storage_data.get());

    // Create a SensorData from DeviceData
    uint32_t string_length = raw_data->data.string_message_length;
    uint32_t length = sizeof(data::Dummy) + raw_data->data.string_message_length;
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::Dummy, length);
    auto dummy_sensor_data = static_cast<data::Dummy*>(sensor_data.get());

    // Parse Data
    dummy_sensor_data->timestamp_intrinsic_ns = raw_data->get_timestamp_receive_ns();
    dummy_sensor_data->int_value = raw_data->data.int_value;
    dummy_sensor_data->string_length = string_length;
    memcpy(dummy_sensor_data->string_buf, &(raw_data->data.string_message), string_length);

    return sensor_data;
}

}  // namespace foobar
}  // namespace dummy
}  // namespace device
}  // namespace hera
}  // namespace wayz