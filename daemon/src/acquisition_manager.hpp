//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include "common/hera_errno.h"
#include "common/logger/logger.hpp"
#include "common/rpc/gen-cpp/AcquisitionManager.h"
#include "common/third_party/json.hpp"
#include "devices/src/device_factory.hpp"

using json = nlohmann::json;

namespace wayz {
namespace hera {
namespace daemon {

class AcquisitionManager final : public AcquisitionManagerIf {
public:
    AcquisitionManager() : record_(false), inited_(false)
    {
        read_profiles();
    }
    virtual ~AcquisitionManager()
    {
        reset();
    }

    // Implementation
    void get(Result& result) override;
    void start(Result& result,
               const std::vector<DeviceInitializer>& devices,
               const std::string& storage_folder) override;
    void stop(Result& result) override;
    void record(Result& result, const bool on) override;
    void adjust(Result& result,
                const int32_t id,
                const std::map<std::string, std::string>& parameters) override;

    void reset();

    // Profile
    void getProfile(ProfileResult& result) override;
    void updateProfile(ProfileResult& result, const std::string& json_str) override;

private:
    void handle_error(Result& result,
                      HeraErrno hera_errno,
                      std::string&& reason = "",
                      bool die = false);
    void handle_success(Result& result);
    void append_status(Result& result);
    void read_profiles();

private:
    std::vector<DevicePtr> devices_;
    bool record_;
    bool inited_;

    static const std::string FolderPrefix_;
    std::string folder_;

    static const std::string JsonFile_;
    json json_instance_;
};

}  // namespace daemon
}  // namespace hera
}  // namespace wayz