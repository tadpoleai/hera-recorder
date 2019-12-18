//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <future>
#include <regex>
#include <thread>

#include "acquisition_manager.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void AcquisitionManager::get(Result& result, const bool is_data)
{
    append_status(result, is_data);
}

void AcquisitionManager::append_status(Result& result, const bool is_data)
{
    result.status.inited = inited_;
    result.status.record = record_;
    result.status.folder = folder_;

    std::vector<std::future<DeviceInformation>> promises;
    // clang-format off
    for (auto& device : devices_) {
        promises.emplace_back(std::async(std::launch::async, [result, is_data](auto* device) {
            DeviceInformation info;
            info.id = device->get_id();
            info.type = device->get_type();
            info.name = device->get_name();
            info.status = static_cast<int32_t>(device->get_status());
            info.forward = false;
            info.volume = device->get_volume() / 1024;
            info.error = device->get_errno();
            info.reason = device->get_reason();

            info.data.valid = false;
            if (is_data || true) {
                auto disp_data = DisplayData::create_from(device->history());
                info.data.valid = disp_data->is_valid;
                info.data.isJpeg = disp_data->is_jpeg;
                info.data.sequence = disp_data->sequence;
                info.data.timeSecond = disp_data->timestamp_intrinsic_ns / OneSecond;
                info.data.timeNanosecond = disp_data->timestamp_intrinsic_ns % OneSecond;
                info.data.data = std::move(disp_data->data);
            }
            return info;
        },
        device.get()));
        }
    // clang-format on
    for (auto& promise : promises) {
        result.status.devices.emplace_back(promise.get());
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz