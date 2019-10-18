//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include "../converter.hpp"

namespace wayz {
namespace tron {

class ImuConverter final : public Converter {
public:
    ImuConverter(const std::string& frame_id,
                 const ::std::string device_name,
                 const std::string& device_data_folder);
    ImuConverter(const ImuConverter&) = delete;
    ImuConverter& operator=(const ImuConverter&) = delete;
    virtual ~ImuConverter();

private:
    bool convert_and_write_one_data(const std::shared_ptr<DeviceRawData>& rawdata) final;

    std::string imu_topic_name_;
    std::string magnetic_topic_name_;
};

}  // namespace tron
}  // namespace wayz