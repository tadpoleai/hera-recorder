//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include "common/hera_errno.h"
#include "common/logger/logger.hpp"
#include "common/third_party/json.hpp"
#include "daemon/rpc/gen-cpp/AcquisitionManager.h"
#include "device/include.hpp"

using json = nlohmann::json;

namespace wayz {
namespace hera {
namespace daemon {

class AcquisitionManager final : public AcquisitionManagerIf {
public:
    AcquisitionManager(const std::string& filename_prefix = "./", const std::string& json_file = "./profiles.json") :
        record_(false),
        inited_(false),
        FileNamePrefix_(filename_prefix),
        FileNameSuffix_(".her"),
        JsonFile_(json_file)
    {
        read_profiles();
    }
    virtual ~AcquisitionManager()
    {
        reset();
    }

    // Control
    void start(Result& result,
               const std::vector<DeviceInitializer>& devices,
               const std::string& storage_folder) override;
    void stop(Result& result) override;
    void record(Result& result, const bool on) override;
    void adjust(Result& result, const int32_t id, const std::map<std::string, std::string>& parameters) override;

    void reset();

    // Profile
    void getProfile(ProfileResult& result) override;
    void updateProfile(ProfileResult& result, const std::string& json_str) override;

    // Data
    void get(Result& result, const bool is_data) override;

private:
    void handle_error(Result& result, HeraErrno hera_errno, std::string&& reason = "", bool die = false);
    void handle_success(Result& result);

    void append_status(Result& result, const bool is_data = false);

    void read_profiles();
    void get_single_data();

private:
    std::vector<device::DevicePtr> devices_;
    bool record_;
    bool inited_;

    const std::string FileNamePrefix_;
    const std::string FileNameSuffix_;
    std::string filename_;
    storage::StorageManagerPtr storage_;

    const std::string JsonFile_;
    json json_instance_;
};

}  // namespace daemon
}  // namespace hera
}  // namespace wayz