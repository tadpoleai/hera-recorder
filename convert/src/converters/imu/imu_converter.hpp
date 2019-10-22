//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include "../converter.hpp"

namespace wayz {
namespace tron {

class ImuConverter final : public Converter {
public:
    ImuConverter(const std::string& device_type,
                 const std::string& device_name,
                 const std::string& device_data_folder,
                 ConverterHandler* handler);
    ImuConverter(const ImuConverter&) = delete;
    ImuConverter& operator=(const ImuConverter&) = delete;
    virtual ~ImuConverter();

private:
    bool convert_one_data(const std::shared_ptr<DeviceRawData>& raw_data) final;
    std::string imu_topic_name_;
    std::string magnetic_topic_name_;
};

}  // namespace tron
}  // namespace wayz