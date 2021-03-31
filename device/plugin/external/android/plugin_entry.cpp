///
/// @file android.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-07-29
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "plugin_common.hpp"
#include "plugin_data.hpp"
#include "plugin_param.hpp"

#include "data/camera_data.hpp"
#include "data/gnss_data.hpp"
#include "data/imu_data.hpp"
#include "data/lidar_data.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace external {
namespace android {

///
/// @brief A dummy device foobar, for sample, Derived from Device
///
HERA_PLUGIN_DEFINE_START(5)

HERA_PLUGIN_DEFINE_END

HERA_PLUGIN_EXPORT(ExternalAndroid, "external/android")

data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameter)
{
    if (storage_data->is_type(DeviceDataType::ExternalAndroidCompressedImage)) {
        // Raw DeviceData of Derived Type
        auto raw_data = static_cast<AndroidCompressedImage*>(storage_data.get());

        // Create a SensorData from DeviceData
        uint32_t image_data_size = raw_data->image_data_size;
        uint32_t length = sizeof(data::CompressedImage) + image_data_size;
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::CompressedImage, length);
        auto camera_sensor_data = static_cast<data::CompressedImage*>(sensor_data.get());

        // Parse Data
        camera_sensor_data->timestamp_intrinsic_ns = raw_data->get_timestamp_receive_ns();
        camera_sensor_data->compress_format = raw_data->compress_format;
        camera_sensor_data->image_data_size = image_data_size;
        memcpy(camera_sensor_data->image_data, &(raw_data->image_data), image_data_size);
        return sensor_data;
    } else if (storage_data->is_type(DeviceDataType::ExternalAndroidImuMagneticField)) {
        // Raw DeviceData of Derived Type
        auto raw_data = static_cast<AndroidImuMagneticField*>(storage_data.get());

        // Create a SensorData from DeviceData
        auto length = sizeof(data::ImuMagneticField);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::ImuMagneticField, length);
        auto imu_sensor_data = static_cast<data::ImuMagneticField*>(sensor_data.get());

        // Parse Data
        imu_sensor_data->timestamp_intrinsic_ns = raw_data->get_timestamp_receive_ns();
        for (auto i = 0; i < 3; ++i) {
            imu_sensor_data->angular_velocity[i] = raw_data->angular_velocity[i];
            imu_sensor_data->linear_acceleration[i] = raw_data->linear_acceleration[i];
            imu_sensor_data->magnetic_field[i] = raw_data->magnetic_field[i];
        }

        return sensor_data;
    } else if (storage_data->is_type(DeviceDataType::ExternalAndroidLocation)) {
        // Raw DeviceData of Derived Type
        auto raw_data = static_cast<AndroidLocation*>(storage_data.get());

        // Create a SensorData from DeviceData
        auto length = sizeof(data::NavSatFix);
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::NavSatFix, length);
        auto nav_sensor_data = static_cast<data::NavSatFix*>(sensor_data.get());

        // Parse Data
        nav_sensor_data->timestamp_intrinsic_ns = raw_data->get_timestamp_receive_ns();
        nav_sensor_data->longitude = raw_data->longitude;
        nav_sensor_data->latitude = raw_data->latitude;
        nav_sensor_data->altitude = raw_data->altitude;
        nav_sensor_data->status.status = data::NavSatFix::StatusType::FIX;
        for (auto i = 1; i < 9; ++i) {
            nav_sensor_data->position_covariance[i] = 0;
        }
        nav_sensor_data->position_covariance[0] = raw_data->horizontal_accuracy;
        nav_sensor_data->position_covariance[4] = raw_data->horizontal_accuracy;
        nav_sensor_data->position_covariance[8] = raw_data->vertical_accuracy;
        nav_sensor_data->position_covariance_type = data::NavSatFix::PositionCovarianceType::DiagonalKnown;

        return sensor_data;
    } else if (storage_data->is_type(DeviceDataType::ExternalAndroidLaserScan)) {
        // Raw DeviceData of Derived Type
        auto raw_data = static_cast<AndroidLaserScan*>(storage_data.get());

        // Create a SensorData from DeviceData
        uint32_t ranges_intensities_length = raw_data->ranges_length + raw_data->intensities_length;
        uint32_t length = sizeof(data::LaserScan) + sizeof(float) * ranges_intensities_length;
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::LaserScan, length);
        auto lidar_sensor_data = static_cast<data::LaserScan*>(sensor_data.get());

        // Parse Data
        lidar_sensor_data->timestamp_intrinsic_ns = raw_data->get_timestamp_receive_ns();
        lidar_sensor_data->angle_min = raw_data->angle_min;
        lidar_sensor_data->angle_max = raw_data->angle_max;
        lidar_sensor_data->angle_increment = raw_data->angle_increment;

        lidar_sensor_data->time_increment = raw_data->time_increment;
        lidar_sensor_data->scan_time = raw_data->scan_time;

        lidar_sensor_data->range_min = raw_data->range_min;
        lidar_sensor_data->range_max = raw_data->range_max;

        lidar_sensor_data->ranges_length = raw_data->ranges_length;
        lidar_sensor_data->intensities_length = raw_data->intensities_length;
        memcpy(&lidar_sensor_data->ranges_intensities_data,
               &raw_data->ranges_intensities_data,
               sizeof(float) * ranges_intensities_length);

        return sensor_data;
    } else {
        return data::SensorData::broken_data();
    }
}

}  // namespace android
}  // namespace external
}  // namespace device
}  // namespace hera
}  // namespace wayz
