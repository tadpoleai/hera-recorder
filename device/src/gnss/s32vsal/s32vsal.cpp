///
/// @file s32vsal.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2020-04-23
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///
///

#include "s32vsal.hpp"

#include <cmath>
#include <numeric>

namespace wayz {
namespace hera {
namespace device {
namespace gnss {
namespace s32vsal {

const std::vector<DeviceParameterType> S32VSal::EssentialParameterTypes = {DeviceParameterType::SubVendorType};

const std::vector<DeviceParameterType> S32VSal::OptionalParameterTypes = {};

auto _ = DeviceFactory::register_type({.type = DeviceVendorType::GnssS32VSal,
                                       .type_name = "gnss/s32vsal",
                                       .create = &S32VSal::create,
                                       .do_convert = &S32VSal::do_convert,
                                       .essential_parameter_types = S32VSal::EssentialParameterTypes,
                                       .optional_parameter_types = S32VSal::OptionalParameterTypes,
#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VSAL
                                       .implemented = true
#else
                                       .implemented = false
#endif
#else
                                       .implemented = false
#endif
});

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VSAL
HeraErrno S32VSal::connect()
{
    std::string sub_vendor_type;
    try {
        sub_vendor_type = parameters_[DeviceParameterType::SubVendorType];
    } catch (...) {
        return handle_error(HeraErrno::InvalidParameterValue);
    }

    SALStatus err = SUCCESS;
    init_called_ = false;
    if (sub_vendor_type == "UBLOX") {
        gnss_type_ = UBLOX;
        err = SALGNSSInit(gnss_type_, "/etc/conf/drivers/gnss/ubloxgps.conf");
        init_called_ = true;
    } else if (sub_vendor_type == "MPS" || sub_vendor_type == "RTK") {
        gnss_type_ = MPSTNAV;
        err = SALGNSSInit(gnss_type_, "/etc/conf/drivers/gnss/rtkgnss.conf");
        init_called_ = true;
    } else {
        return handle_error(HeraErrno::InvalidParameterValue,
                            "GNSSS32VSal: SubVendorType should be either 'UBLOX' or 'RTK, got '" + sub_vendor_type +
                                    "' instead");
    }

    if (err != SUCCESS) {
        return handle_error(HeraErrno::CanNotOpenTtyDevice, "GNSSS32VSal: Can not open s32vsal device");
    }

    return HeraErrno::Success;
}

/// Free the serial port object
///
void S32VSal::disconnect()
{
    if (init_called_) {
        // SALGNSSDeInit(gnss_type_);
    }
}

/// Fetch data from serial port
///
data::DeviceDataPtr S32VSal::fetch()
{
    SGNSSNavigation nav;
    auto err = SALGNSSReadNavigation(gnss_type_, &nav, 50);

    if (err != SUCCESS) {
        return nullptr;
    }

    auto length = sizeof(S32VSalData);
    auto data = data::DeviceData::create(length,
                                         id_,
                                         DeviceVendorType::GnssS32VSal,
                                         DeviceDataType::GnssS32VSalData,
                                         sequence_++);
    auto derived_data = static_cast<S32VSalData*>(data.get());

    derived_data->data.timestamp_intrinsic_ns = time::OneSecond * nav.timeStamp.tv_sec + 1000LL * nav.timeStamp.tv_usec;
    derived_data->data.latitude = nav.latitude;
    derived_data->data.longitude = nav.longitude;
    derived_data->data.altitude = nav.altitude;
    derived_data->data.speed = nav.speed;
    derived_data->data.quality = static_cast<S32VSalData::QualityType>(nav.signal);
    derived_data->data.pdop = nav.pdop;
    derived_data->data.hdop = nav.hdop;
    derived_data->data.vdop = nav.vdop;
    derived_data->data.latitude_valid = (nav.flags & GNSS_LAT) != 0;
    derived_data->data.longitude_valid = (nav.flags & GNSS_LON) != 0;
    derived_data->data.altitude_valid = (nav.flags & GNSS_ALT) != 0;
    derived_data->data.speed_valid = (nav.flags & GNSS_SPEED) != 0;
    derived_data->data.pdop_valid = (nav.flags & GNSS_PDOP) != 0;
    derived_data->data.hdop_valid = (nav.flags & GNSS_HDOP) != 0;
    derived_data->data.vdop_valid = (nav.flags & GNSS_VDOP) != 0;
    return data;
}

HeraErrno S32VSal::adjust_parameter(DeviceParameterType type, const std::string& value)
{
    switch (type) {
    case DeviceParameterType::DataPort:
        return HeraErrno::ImmutableParameter;
    default:
        return HeraErrno::UnimplementedParameter;
    }
    return HeraErrno::Success;
}
#endif
#endif

data::SensorDataPtr S32VSal::do_convert(data::DeviceDataPtr& storage_data)
{
    if (!storage_data->is_type(DeviceDataType::GnssS32VSalData)) {
        return data::SensorData::broken_data();
    }

    // Raw DeviceData of Derived Type
    auto raw_data = static_cast<S32VSalData*>(storage_data.get());

    if (!raw_data->data.latitude_valid || !raw_data->data.longitude_valid || !raw_data->data.altitude_valid) {
        log::warn << "GnssS32VSal: Get invalid data" << log::endl;
    }

    // Create a SensorData from DeviceData
    auto length = sizeof(data::NavSatFix);
    auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::NavSatFix, length);
    auto navsatfix_sensor_data = static_cast<data::NavSatFix*>(sensor_data.get());

    // Initialize an invalid data template
    if (raw_data->data.quality == S32VSalData::QualityType::RTKFixed) {
        navsatfix_sensor_data->status.status = data::NavSatFix::StatusType::FIX;
    } else if (raw_data->data.quality == S32VSalData::QualityType::RTKFloating) {
        navsatfix_sensor_data->status.status = data::NavSatFix::StatusType::NO_Fix;
    } else {
        navsatfix_sensor_data->status.status = data::NavSatFix::StatusType::NO_Fix;
    }

    navsatfix_sensor_data->status.service = data::NavSatFix::ServiceType::GPS;
    navsatfix_sensor_data->position_covariance_type = data::NavSatFix::PositionCovarianceType::Unknown;
    navsatfix_sensor_data->position_covariance[0] = -1;
    for (auto i = 1; i < 9; ++i) {
        navsatfix_sensor_data->position_covariance[i] = 0;
    }

    navsatfix_sensor_data->latitude = raw_data->data.latitude;
    navsatfix_sensor_data->longitude = raw_data->data.longitude;
    navsatfix_sensor_data->altitude = raw_data->data.altitude;
    navsatfix_sensor_data->timestamp_intrinsic_ns = raw_data->data.timestamp_intrinsic_ns;

    log::debug << "GnssS32VSal: Get data: t = " << navsatfix_sensor_data->timestamp_intrinsic_ns << log::endl;

    return sensor_data;
}

}  // namespace s32vsal
}  // namespace gnss
}  // namespace device
}  // namespace hera
}  // namespace wayz
