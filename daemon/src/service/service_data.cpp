//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <future>
#include <regex>
#include <thread>

#include "device/include/display_data.hpp"
#include "service.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::append_data_status(DataStatus& result)
{
    std::vector<std::future<DeviceData>> promises;
    // clang-format off
    for (auto& device : devices_) {
        promises.emplace_back(std::async(std::launch::async, [&](auto* device) {
            DeviceData data;
            data.id = device->get_id();
            data.type = device->get_vendor_type();
            data.name = device->get_name();
            data.dataSize = device->get_volume();

            data.health = device->get_health();
            data.statusMessage = device->get_status_message();

            const bool is_detail_disp_data = (data.id == detail_device_index_);
            const auto disp_datas = device->update_get_disp_data(is_detail_disp_data);
            for (const auto& category: disp_datas->categorized_disp_data) {
                daemon::SingleDisplayData single_data;
                single_data.jpegData = category.second.jpeg_data;
                single_data.textData = category.second.text_data;
                data.dispData.emplace_back(std::move(single_data));
            }
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
            result.slamResult.resize(slam_result_->data.size());
            memcpy((void*)result.slamResult.data(), slam_result_->data.data(), slam_result_->data.size());
        }
    }
#endif

    for (auto& promise : promises) {
        result.deviceDatas.emplace_back(promise.get());
    }

    for (size_t i = 0; i < result.deviceDatas.size(); ++i) {
        result.deviceDatas[i].frequency = frequecy_calculator_->get_result(i);
    }

    result.startTimeSec = start_time_sec_;
    result.nowTimeSec = time::Timestamp::now().tv_sec;
}

void Service::getData(DataStatus& result)
{
    if (!mutex_.try_lock()) {
        result.error = 1;
        result.reason = "Other operation busy";
        return;
    }

    append_data_status(result);

    mutex_.unlock();
}

void Service::selectDetailDevice(DataStatus& result, const int32_t deviceIndex)
{
    std::unique_lock<std::mutex> _(mutex_);

    detail_device_index_ = deviceIndex;

    append_data_status(result);
}

void Service::clearDetailDevice(DataStatus& result)
{
    std::unique_lock<std::mutex> _(mutex_);

    detail_device_index_ = -1;

    append_data_status(result);
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
