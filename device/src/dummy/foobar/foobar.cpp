///
/// @file foobar.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Foobar
/// @version 0.1
/// @date 2019-11-07
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "foobar.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>

namespace wayz {
namespace hera {
namespace device {
namespace dummy {
namespace foobar {

const std::vector<DeviceParameterType> Foobar::EssentialParameterTypes = {DeviceParameterType::DummyRate,
                                                                          DeviceParameterType::DummyValue};

const std::vector<DeviceParameterType> Foobar::OptionalParameterTypes = {};

auto _ = DeviceFactory::register_type({.type = DeviceVendorType::DummyFoobar,
                                       .type_name = "dummy/foobar",
                                       .create = &Foobar::create,
                                       .do_convert = &Foobar::do_convert,
                                       .essential_parameter_types = Foobar::EssentialParameterTypes,
                                       .optional_parameter_types = Foobar::OptionalParameterTypes,
                                       .implemented = true});

const std::string Foobar::DefaultMessage_ = "This is a default message from dummy sensor";

/// Parse parameters first
/// @note For parameters delcared in essential parameters,
/// just parse it.
/// @note For parameters not delcared in essential parameters,
/// just existance first, then parse it
/// @note For non-string value, use stoi, stol, stod inside a try-catch block
///
/// Then allocate resource and connect
HeraErrno Foobar::connect()
{
    try {
        int_value_ = stoi(parameters_[DeviceParameterType::DummyValue]);
        period_us_ = 1'000'000 / stoi(parameters_[DeviceParameterType::DummyRate]);
    } catch (...) {
        return handle_error(HeraErrno::InvalidParameterValue);
    }

    // Parameter with default value
    if (parameters_.count(DeviceParameterType::DummyString)) {
        string_message_ = DeviceParameterType::DummyString;
    } else {
        string_message_ = DefaultMessage_;
    }

    // Open some resources and handlers
    // ...

    return HeraErrno::Success;
}

/// Free resources and handlers opened by connect()
///
void Foobar::disconnect() {}


/// See source for a code sample of,
/// how to create and return a valid data
data::DeviceDataPtr Foobar::fetch()
{
    // Foobar::fetch() blocks when fetching for a certain period
    std::this_thread::sleep_for(std::chrono::microseconds(period_us_));

    // Some devices randomly fails during fetching,
    // in case of that, return a nullptr
    if (std::rand() > RAND_MAX * 0.9) {
        return nullptr;
    }

    log::debug << "Foobar: Generating dummy data" << log::endl;

    // Length of variable-lengthed-buf fields, in bytes
    auto string_length = string_message_.size();
    // Total length of device data
    auto length = sizeof(FoobarData) + string_length;
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::DummyFoobar,
                                         DeviceDataType::DummyFoobarData,
                                         sequence_++);
    auto derived_data = static_cast<FoobarData*>(data.get());

    // Set non-buf-typed fields
    derived_data->data.int_value = int_value_;
    for (auto i = 0; i < 4; ++i) {
        derived_data->data.char_value[i] = int_value_ % 0x100;
    }

    // Set length of variable-lengthed-buf fields' length. in bytes
    derived_data->data.string_message_length = string_length;
    // Use Memcpy to buf-typed fields
    memcpy(derived_data->data.string_message, string_message_.data(), string_length);

    return data;
}

/// See source for a code sample of,
/// how to implement this function
/// @note For a mutable parameter, apply the new value
/// @note For an immutable, return HeraErrno::ImmutableParameter
/// @note Return HeraErrno::UnimplementedParameter by default
HeraErrno Foobar::adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::DummyValue: {
        try {
            int_value_ = stoi(value);
        } catch (...) {
            return HeraErrno::InvalidParameterValue;
        }
        return HeraErrno::OK;
        break;
    }
    case DeviceParameterType::DummyRate: {
        try {
            period_us_ = 1'000'000 / stoi(value);
        } catch (...) {
            return HeraErrno::InvalidParameterValue;
        }
        return HeraErrno::OK;
        break;
    }
    case DeviceParameterType::DummyString: {
        string_message_ = value;
        return HeraErrno::OK;
        break;
    }
    default:
        return HeraErrno::UnimplementedParameter;
    }
}

/// See source for a code sample of,
/// how to create and return a valid converted data
/// @note Check the type of device data first,
data::SensorDataPtr Foobar::do_convert(data::DeviceDataPtr& storage_data)
{
    if (!storage_data->is_type(DeviceDataType::DummyFoobarData)) {
        return data::SensorData::broken_data();
    }

    log::debug << "Foobar: Converting dummy data" << log::endl;

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