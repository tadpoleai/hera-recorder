///
/// @file image.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Image
/// @version 0.1
/// @date 2020-04-13
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "image.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>

#include "../../plugin_impl.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace dummy {
namespace image {

const std::vector<std::string> Image::EssentialParameterTypes = {"DummyRate"};

const std::vector<std::string> Image::OptionalParameterTypes = {};

HERA_DEVICE_DRIVER_EXPORT_PLUGIN(DummyImage, "dummy/image", Image)

#ifdef WITH_DRIVER

/// Parse parameters first
/// @note For parameters delcared in essential parameters,
/// just parse it.
/// @note For parameters not delcared in essential parameters,
/// just existance first, then parse it
/// @note For non-string value, use stoi, stol, stod inside a try-catch block
///
/// Then allocate resource and connect
HeraErrno Image::connect()
{
    try {
        period_us_ = 1'000'000 / stoi(parameters_["DummyRate"]);
    } catch (...) {
        return handle_error(HeraErrno::InvalidParameterValue);
    }

    // Open some resources and handlers
    // ...

    return HeraErrno::Success;
}

/// Free resources and handlers opened by connect()
///
void Image::disconnect() {}


/// See source for a code sample of,
/// how to create and return a valid data
data::DeviceDataPtr Image::fetch()
{
    // Image::fetch() blocks when fetching for a certain period
    std::this_thread::sleep_for(std::chrono::microseconds(period_us_));

    // Some devices randomly fails during fetching,
    // in case of that, return a nullptr
    if (std::rand() > RAND_MAX * 0.9) {
        return nullptr;
    }

    log::debug << "Image: Generating dummy image data" << log::endl;

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
HeraErrno Image::adjust_parameter(const std::string& type, const std::string& value)
{
    if (type == "DummyRate") {
        try {
            period_us_ = 1'000'000 / stoi(value);
        } catch (...) {
            return HeraErrno::InvalidParameterValue;
        }
        return HeraErrno::OK;
    }

    return HeraErrno::UnimplementedParameter;
}

#endif

/// See source for a code sample of,
/// how to create and return a valid converted data
/// @note Check the type of device data first,
data::SensorDataPtr Image::do_convert(data::DeviceDataPtr& storage_data)
{
    if (!storage_data->is_type(DeviceDataType::DummyImageData)) {
        return data::SensorData::broken_data();
    }

    log::debug << "Image: Converting dummy data" << log::endl;

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