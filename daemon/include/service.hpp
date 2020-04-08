//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "common/include/hera_errno.h"
#include "common/include/logger/logger.hpp"
#include "common/include/third_party/json.hpp"
#include "daemon/rpc/gen-cpp/Service.h"
#include "device/include/include.hpp"

#ifdef WITH_SLAM
#include "slam/caller/include/caller.hpp"
#include "slam/result/include/result.hpp"
#endif

using json = nlohmann::json;

namespace wayz {
namespace hera {
namespace daemon {

class Service final : public ServiceIf {
public:
    Service(const std::string& filename_prefix = "./", const std::string& profiles_filename = "./profiles.json") :
        started_(false),
        recording_(false),
        FileNamePrefix_(filename_prefix),
        FileNameSuffix_(".her"),
        ProfilesFileName_(profiles_filename)
    {
        generate_meta();
        load_profiles();
#ifdef WITH_SLAM
        slam_handler_ = std::move(slam::Result::handler());
        slam_result_ = nullptr;
#endif
    }

    virtual ~Service()
    {
        reset();
    }

    void get(Result& _return) override;

    void start(Result& _return, const int32_t profileIndex, const std::string& storageName) override;

    void stop(Result& _return) override;

    void record(Result& _return, const bool on) override;

    void adjustParameters(Result& _return, const int32_t id, const std::vector<Parameter>& parameters) override;

    void updateProfiles(Result& _return, const std::vector<Profile>& profiles) override;

    void getData(ResultData& _return) override;

    void reset();

private:
    void handle_error(Result& _return, HeraErrno hera_errno, std::string&& reason = "", bool die = false);

    void handle_success(Result& _return);

    void append_status(Result& _return);

    void generate_meta();

    void load_profiles();

    void dump_profiles();

    void get_single_data();

private:
    std::mutex mutex_;

    Meta meta_;

    std::vector<device::DevicePtr> devices_;
    bool started_;
    bool recording_;

    const std::string FileNamePrefix_;
    const std::string FileNameSuffix_;
    std::string storage_name_;
    storage::StorageManagerPtr storage_;

    std::unique_ptr<ipc::IPCQueue<device::data::SensorData>> ipc_queue_;

    const std::string ProfilesFileName_;
    std::vector<Profile> profiles_;

#ifdef WITH_SLAM
    decltype(slam::Result::handler()) slam_handler_;
    slam::ResultPtr slam_result_;
#endif
};

}  // namespace daemon
}  // namespace hera
}  // namespace wayz