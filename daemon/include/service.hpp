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
#include "frequecy_calculator.hpp"
#include "serialize.hpp"
#include "storage/include/upload.hpp"

#ifdef WITH_SLAM
#include "slam/caller/include/caller.hpp"
#include "slam/result/include/result.hpp"
#endif


using json = nlohmann::json;

namespace wayz {
namespace hera {
namespace daemon {

///
/// @brief Struct of remote server to upload storage fileI
///
struct RemoteServerType {
    std::string remark;       ///< Remark(name) in client
    std::string protocol;     ///< Protocol, @see storage::upload::UploadProtocol
    std::string destination;  ///< Destination of upload protocol, ip address or remark of ssh
};

class Service final : public ServiceIf {
public:
    Service(const std::string& storage_folder = ".",
            const std::string& setting_filename = "./setting.json",
            const std::vector<RemoteServerType>& remote_servers = {}) :
        StorageFolder_(storage_folder),
        FileNameSuffix_(".hera"),
        AcquisitionSettingFileName_(setting_filename),
        started_(false),
        recording_(false),
        start_time_sec_(0),
        remote_servers_(remote_servers)
    {
        generate_meta();
        load_setting();
#ifdef WITH_SLAM
        slam_handler_ = std::move(slam::Result::handler());
        slam_result_ = nullptr;
#endif
    }

    virtual ~Service()
    {
        reset();
    }

    virtual void getMeta(Meta& _return) override;

    virtual void getSetting(AcquisitionSetting& _return) override;
    virtual void setProfiles(AcquisitionSetting& _return, const std::vector<Profile>& profiles) override;
    virtual void selectProfile(AcquisitionSetting& _return, const int32_t profileIndex) override;
    virtual void setOperatorInfo(AcquisitionSetting& _return, const OperatorInfo& operatorInfo) override;

    virtual void getStatus(AcquisitionStatus& _return) override;
    virtual void start(AcquisitionStatus& _return) override;
    virtual void stop(AcquisitionStatus& _return) override;
    virtual void setRecord(AcquisitionStatus& _return, const bool on) override;

    virtual void getData(DataStatus& _return) override;
    virtual void selectDetailDevice(DataStatus& _return, const int32_t deviceIndex) override;
    virtual void clearDetailDevice(DataStatus& _return) override;

    virtual void getDeviceAndParameterses(std::vector<DeviceAndParameters>& _return) override;
    virtual void adjustDeviceParameter(std::vector<DeviceAndParameters>& _return,
                                       const int32_t deviceIndex,
                                       const std::string& type,
                                       const std::string& value) override;

    virtual void getDiskUsageStatus(DiskUsageStatus& _return) override;

    virtual void getStorage(std::vector<StorageRecordFile>& _return) override;
    virtual void deleteStorage(std::vector<StorageRecordFile>& _return, const std::string& name) override;

    virtual void getUploadServers(std::vector<std::string>& _return) override;
    virtual void getUploadProcesses(std::vector<UploadProcess>& _return) override;
    virtual void requestUpload(std::vector<UploadProcess>& _return, const UploadRequest& request) override;

    void reset();

private:
    void generate_meta();

    void load_setting();
    void dump_setting();
    void append_acquisition_setting(AcquisitionSetting& _return);

    void handle_error(AcquisitionStatus& _return, HeraErrno hera_errno, std::string&& reason = "", bool die = false);
    void handle_success(AcquisitionStatus& _return);
    void append_acquisition_status(AcquisitionStatus& _return);

    void append_data_status(DataStatus& _return);

    void append_device_parameterses(std::vector<DeviceAndParameters>& _return);

    void append_storage_status(std::vector<StorageRecordFile>& _return);

    void append_upload_processes(std::vector<UploadProcess>& _return);

private:
    const std::string StorageFolder_;
    const std::string FileNameSuffix_;
    const std::string AcquisitionSettingFileName_;

private:
    std::mutex mutex_;

    // Meta
    Meta meta_;

    // Setting
    AcquisitionSetting acquisition_setting_;

    // Control & Data
    std::vector<device::DevicePtr> devices_;
    int32_t detail_device_index_;
    std::string storage_filename_;
    storage::StorageManagerPtr storage_;
    std::unique_ptr<ipc::IPCQueue<device::data::SensorData>> ipc_queue_;
    std::unique_ptr<FrequecyCalculator> frequecy_calculator_;
    bool started_;
    bool recording_;
    int32_t start_time_sec_;
#ifdef WITH_SLAM
    decltype(slam::Result::handler()) slam_handler_;
    slam::ResultPtr slam_result_;
#endif

    // Storage & Upload
    StorageStatus storage_status_;
    std::vector<std::unique_ptr<storage::upload::Manager>> upload_managers_;
    std::vector<RemoteServerType> remote_servers_;
};

}  // namespace daemon
}  // namespace hera
}  // namespace wayz