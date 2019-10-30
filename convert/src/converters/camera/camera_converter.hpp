//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include "../converter.hpp"

namespace wayz {
namespace tron {

class CameraConverter final : public Converter {
public:
    CameraConverter(const std::string& device_type,
                    const std::string& device_name,
                    const std::string& device_data_folder,
                    const std::string& optional_frame_id,
                    const std::vector<std::string>& optional_topics,
                    ConverterHandler* handler);
    CameraConverter(const CameraConverter&) = delete;
    CameraConverter& operator=(const CameraConverter&) = delete;
    virtual ~CameraConverter();

private:
    bool convert_one_data(const std::shared_ptr<DeviceRawData>& raw_data) final;

    std::string image_topic_name_;
};

}  // namespace tron
}  // namespace wayz