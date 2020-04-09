///
/// @file device.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class Device
/// @version 0.1
/// @date 2019-11-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "device.hpp"

namespace wayz {
namespace hera {
namespace device {

Device::Device(const uint32_t id,
               const std::string& vendor_type,
               const std::string& name,
               const bool forward,
               ipc::IPCQueue<data::SensorData>* const ipc_queue,
               storage::StorageManager* const storage,
               const size_t history_depth,
               const std::vector<DeviceParameterType>& essential_parameter_types) :
    id_(id),
    vendor_type_(vendor_type),
    name_(name),
    sequence_(0),
    history_depth_(history_depth),
    status_(DeviceStatus::BeforeConnect),
    hera_errno_(HeraErrno::OK),
    is_record_(false),
    thread_fetch_(nullptr),
    storage_(storage),
    is_forward_(forward),
    thread_forward_(nullptr),
    ipc_queue_(ipc_queue),
    essential_parameter_types_(essential_parameter_types)
{
    if (storage_) {
        storage_->add_device(vendor_type_ + "/" + name_, history_depth_);
    }
}

Device::~Device() {}

/// Start to connect to device.
/// If device is in DeviceStatus::BeforeConnect,
/// this function will check essential parameters first,
/// then call abstract function connect(),
/// after that create a storage object.
/// If device is in other status, return corresponding error code
HeraErrno Device::start()
{
    switch (status_) {
    case DeviceStatus::BeforeConnect: {
        if (!check_parameter()) {
            return HeraErrno::InsufficientParameters;
        }
        auto result = connect();
        if (result != HeraErrno::OK) {
            return result;
        }
        status_ = DeviceStatus::Connected;
        thread_fetch_ = new std::thread(&Device::fetch_thread_function, this);
        if (is_forward_) {
            thread_forward_ = new std::thread(&Device::forward_thread_function, this);
        }
        return HeraErrno::OK;
    }

    case DeviceStatus::Connected:
        return HeraErrno::DeviceAlreadyConnected;

    case DeviceStatus::Terminated:
        return HeraErrno::DeviceAlreadyClosed;

    case DeviceStatus::Error:
    default:
        return HeraErrno::InStatusError;
    }
}

/// Stop all resources and thread in this device.
/// If device is not in DeviceStatus::Terminated,
/// then set status to DeviceStatus::Terminated,
/// and then stop recording and join fetching thread,
/// after that, close storage
void Device::stop()
{
    if (status_ != DeviceStatus::Terminated) {
        status_ = DeviceStatus::Terminated;
        is_record_ = false;
        if (thread_fetch_ != nullptr) {
            thread_fetch_->join();
            delete thread_fetch_;
            thread_fetch_ = nullptr;
        }
        if (thread_forward_ != nullptr) {
            thread_forward_->join();
            delete thread_forward_;
            thread_forward_ = nullptr;
        }
        disconnect();
    }
}

/// Change is_record_.
/// If device status is DeviceStatus::Connected,
/// set new value of is_record_,
/// otherwise, return corresponding error code
HeraErrno Device::record(bool value)
{
    switch (status_) {
    case DeviceStatus::BeforeConnect:
        return HeraErrno::DeviceNotReady;

    case DeviceStatus::Connected:
        is_record_ = value;
        return HeraErrno::OK;

    case DeviceStatus::Terminated:
        return HeraErrno::DeviceAlreadyClosed;

    case DeviceStatus::Error:
    default:
        return HeraErrno::InStatusError;
    }
}

/// Check if argument type(string) in valid,
/// and then add it to protected member parameters_,
/// in additional, call adjust_parameter() if device is connected
HeraErrno Device::parameter(const std::string& type, const std::string& value)
{
    DeviceStatus status = status_;
    if (status == DeviceStatus::Error) {
        return HeraErrno::InStatusError;
    }
    if (status == DeviceStatus::Terminated) {
        return HeraErrno::DeviceAlreadyClosed;
    }
    auto parameter_type = DeviceParameterType::_from_string_nocase_nothrow(type.c_str());
    if (!parameter_type) {
        return HeraErrno::InvalidParameterType;
    } else {
        auto result = HeraErrno::OK;
        if (status == DeviceStatus::Connected) {
            result = adjust_parameter(parameter_type.value(), value);
        }
        if (result == OK) {
            parameters_[parameter_type.value()] = value;
        }
        return result;
    }
}

/// Read history of device data
/// and then convert
std::vector<data::SensorDataPtr> Device::history()
{
    auto sensor_datas = std::vector<data::SensorDataPtr>();
    sensor_datas.reserve(history_depth_);
    auto storage_datas = storage_->history(id_);
    for (auto&& storage_data : storage_datas) {
        if (storage_data) {
            auto sensor_data = convert(storage_data);
            if (sensor_data->sensor_data_type != SensorDataType::Broken) {
                sensor_datas.emplace_back(std::move(sensor_data));
            }
        }
    }
    return sensor_datas;
}

/// Convert parameters_ from enum:string map
/// to string:string map
OutParametersMap Device::get_parameters() const
{
    OutParametersMap parameters;
    for (const auto& parameter : parameters_) {
        parameters[parameter.first._to_string()] = parameter.second;
    }
    return parameters;
}

/// Set status to DeviceStatus::Error, and set reason
///
HeraErrno Device::handle_error(HeraErrno e, std::string&& reason)
{
    status_ = DeviceStatus::Error;
    hera_errno_ = e;
    reason_ = std::forward<std::string>(reason);
    return e;
}

/// Check if every parameter_type in essential_parameter_types
/// is settled with define_parameter() by testing parameters
bool Device::check_parameter()
{
    for (const auto& type : essential_parameter_types_) {
        if (!parameters_.count(type)) {
            return false;
        }
    }
    return true;
}

/// Check the status, if in DeviceStatus::Connected
/// repeatly call abstract function fetch(),
/// if it returns a non-nullptr value, pass it to storage
void Device::fetch_thread_function()
{
    while (status_ == DeviceStatus::Connected) {
        auto new_data = fetch();

        if (new_data != nullptr) {
            if (is_forward_) {
                forward_queue_.push(new_data);
            }
            if (storage_) {
                storage_->add_data(id_, new_data, is_record_);
            }
        }
    }
}

/// Check the status, if in DeviceStatus::Connected
/// repeatly pop data from storaged data
/// if it returns a non-nullptr value, pass it to storage
void Device::forward_thread_function()
{
    while (status_ == DeviceStatus::Connected) {
        auto storage_data = forward_queue_.wait_pop();
        if (storage_data != nullptr) {
            auto sensor_data = convert(storage_data);
            if (sensor_data->sensor_data_type != SensorDataType::Broken) {
                ipc_queue_->write(sensor_data);
            }
        }
    }
}

}  // namespace device
}  // namespace hera
}  // namespace wayz