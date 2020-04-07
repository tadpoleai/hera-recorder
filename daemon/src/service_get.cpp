//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <future>
#include <regex>
#include <thread>

#include "service.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::get(Result& result)
{
    log::debug << "Daemon::get called" << log::endl;
    std::unique_lock<std::mutex> _(mutex_);
    append_status(result);
}

void Service::append_status(Result& result)
{
    result.status.started = started_;
    result.status.recording = recording_;
    result.status.storageName = storage_name_;
    result.status.meta = meta_;

    std::vector<std::future<DeviceStatus>> promises;
    // clang-format off
    for (auto& device : devices_) {
        promises.emplace_back(std::async(std::launch::async, [result](auto* device) {
            DeviceStatus info;
            info.id = device->get_id();
            info.type = device->get_vendor_type();
            info.name = device->get_name();
            info.forward = device->get_forward();
            info.dataSizeKB = device->get_volume() / 1024;
            info.error = device->get_errno();
            info.reason = device->get_reason();

            /*info.data.valid = false;
            if (is_data || true) {
                auto disp_data = device::data::DisplayData::create_from(device->history());
                info.data.valid = disp_data->is_valid;
                info.data.isJpeg = disp_data->is_jpeg;
                info.data.sequence = disp_data->sequence;
                info.data.timeSecond = disp_data->timestamp_intrinsic_ns / time::OneSecond;
                info.data.timeNanosecond = disp_data->timestamp_intrinsic_ns % time::OneSecond;
                info.data.data = std::move(disp_data->data);
            }
            */
            return info;
        },
        device.get()));
        }
    // clang-format on

    try {
        result.status.profiles = profiles_;
    } catch (const std::exception& e) {
        log::error << e.what() << log::endl;
        log::error << "Daemon::Can not get profiles" << log::endl;
    }

#ifdef WITH_SLAM
    if (slam_handler_) {
        auto ret = slam_handler_->read();
        if (ret) {
            slam_result_ = ret;
        }
        if (slam_result_) {
            result.slamResult.valid = true;
            result.slamResult.isJpeg = true;
            result.data = slam_result_->data;
        }
    }
#endif

    for (auto& promise : promises) {
        result.status.devices.emplace_back(promise.get());
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz