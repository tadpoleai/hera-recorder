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

void Service::getData(ResultData& result)
{
    if (!mutex_.try_lock()) {
        result.error = 1;
        result.reason = "Other operation busy";
        return;
    }

    std::vector<std::future<DeviceData>> promises;
    // clang-format off
    for (auto& device : devices_) {
        promises.emplace_back(std::async(std::launch::async, [result](auto* device) {
            DeviceData data;
            data.id = device->get_id();
            data.type = device->get_vendor_type();
            data.name = device->get_name();

            data.valid = false;
            auto disp_data = device::data::DisplayData::create_from(device->history());
            data.valid = disp_data->is_valid;
            data.isJpeg = disp_data->is_jpeg;
            data.sequence = disp_data->sequence;
            data.timeSecond = disp_data->timestamp_intrinsic_ns / time::OneSecond;
            data.timeNanosecond = disp_data->timestamp_intrinsic_ns % time::OneSecond;
            data.data = std::move(disp_data->data);
            return data;
        },
        device.get()));
    }
    // clang-format on

    result.slamResultValid = false;
#ifdef WITH_SLAM
    if (slam_handler_) {
        auto ret = slam_handler_->read();
        if (ret) {
            slam_result_ = ret;
        }
        if (slam_result_) {
            result.slamResultValid = true;
            result.slamResult = slam_result_->data;
        }
    }
#endif

    for (auto& promise : promises) {
        result.deviceDatas.emplace_back(promise.get());
    }

    mutex_.unlock();
}  // namespace daemon

}  // namespace daemon
}  // namespace hera
}  // namespace wayz