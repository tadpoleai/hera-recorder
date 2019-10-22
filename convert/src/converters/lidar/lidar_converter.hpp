//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include "../converter.hpp"

namespace wayz {
namespace tron {

class LidarConverter final : public Converter {
public:
    LidarConverter(const std::string& device_type,
                   const std::string& device_name,
                   const std::string& device_data_folder,
                   ConverterHandler* handler);
    LidarConverter(const LidarConverter&) = delete;
    LidarConverter& operator=(const LidarConverter&) = delete;
    virtual ~LidarConverter();

private:
    bool convert_one_data(const std::shared_ptr<DeviceRawData>& raw_data) final;

    std::string pcl_topic_name_;
};

}  // namespace tron
}  // namespace wayz