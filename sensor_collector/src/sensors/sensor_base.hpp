//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __sensor_base_hpp__
#define __sensor_base_hpp__
#include <cstdint>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <common/data_message/sensor_and_data_types.hpp>
#include <common/third_party/enum.h>
#include <common/tron_errno.h>
#include <common/utils/system_timestamp.hpp>
#include <common/utils/threadsafe_queue.hpp>


namespace wayz {

class SensorBase {
public:
    SensorBase(int32_t id, const std::string& name);
    SensorBase(const SensorBase&) = delete;
    SensorBase& operator=(const SensorBase&) = delete;
    virtual ~SensorBase();

    // Control
    TronErrno startRecord();
    TronErrno pauseRecord();
    TronErrno connect();
    TronErrno terminate();
    TronErrno setStorageFolder(const std::string& folder);
    TronErrno enableForward();
    TronErrno disableForward();
    TronErrno setParameter(const std::string& type, const std::string& value);
    TronErrno adjustParameter(const std::string& type, const std::string& value);

    // Status
    virtual SensorType getType() const = 0;
    int32_t getId() const;
    std::string getName() const;
    std::string getStatus() const;
    TronErrno getDiagnosis() const;

protected:
    TronErrno setError(TronErrno e);
    std::map<SensorParameterType, std::string> parameters_;
    int32_t sequence_;

    // Sensor Dependent Functions
    virtual TronErrno doConnectSensor() = 0;
    virtual void doDisconnectSensor() = 0;
    virtual std::shared_ptr<SensorRawData> doFetchRawData() = 0;
    virtual std::shared_ptr<SensorData> doConvertData(
            const std::shared_ptr<SensorRawData>& rawdata) = 0;
    virtual TronErrno doAdjustParameter(SensorParameterType type, const std::string& value) = 0;

private:
    TronErrno createStorageFolder();
    TronErrno openNewStorageFile();

    void fetchThreadFunc();
    void storageThreadFunc();
    void forwardThreadFunc();
    bool checkNewData() const;

    int32_t id_;
    std::string name_;
    std::string storagePath_;
    bool isStoragePathSet_;
    int64_t fileNumberCounter_;
    int64_t fileSizeCounter_;
    static const int64_t FileMaxSize_ = 0x7FFFFFFFL;
    static const int64_t FileNameWidth = 4;
    std::ofstream file_;

    mutable SensorStatus status_;
    mutable TronErrno latestErrno_;
    mutable TimestampNs latestDataTimestampNs_;
    static const DurationNs MaxDataDurationNs_ = 3 * OneSecondToNs;
    mutable bool isRecordEnabled_;
    mutable bool isForwardEnabled_;

    std::thread* threadFetch_;
    std::thread* threadStorage_;
    std::thread* threadForward_;
    ThreadsafeQueue<std::shared_ptr<SensorRawData>> queueStorage_;
    ThreadsafeQueue<std::shared_ptr<SensorRawData>> queueForward_;
};

}  // namespace wayz
#endif