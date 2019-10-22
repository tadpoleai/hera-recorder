//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include "../converter.hpp"

namespace wayz {
namespace tron {

class LidarConverter final : public Converter {
public:
    LidarConverter(const std::string& frame_id,
                   const std::string& device_name,
                   const std::string& device_data_folder);
    LidarConverter(const LidarConverter&) = delete;
    LidarConverter& operator=(const LidarConverter&) = delete;
    virtual ~LidarConverter();

private:
    bool convert_and_write_one_data(const std::shared_ptr<DeviceRawData>& rawdata) final;

    std::string pcl_topic_name_;
};

}  // namespace tron
}  // namespace wayz