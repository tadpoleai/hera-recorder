//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "sensor_base.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace wayz {

SensorBase::SensorBase(int32_t id, const std::string& name) :
    sequence_(0),
    id_(id),
    name_(name),
    isStoragePathSet_(false),
    fileNumberCounter_(0),
    fileSizeCounter_(0),
    status_(SensorStatus::Uninited),
    latestErrno_(TronErrno::Success),
    latestDataTimestampNs_(0),
    isRecordEnabled_(false),
    isForwardEnabled_(false)

{
    threadFetch_ = new std::thread(&SensorBase::fetchThreadFunc, this);
    threadStorage_ = new std::thread(&SensorBase::storageThreadFunc, this);
    threadForward_ = new std::thread(&SensorBase::forwardThreadFunc, this);
}

SensorBase::~SensorBase()
{
    terminate();
}

TronErrno SensorBase::startRecord()
{
    auto status = status_;
    if (status == SensorStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (status == SensorStatus::Inited) {
        if (!isStoragePathSet_) {
            createStorageFolder();
        }
        isRecordEnabled_ = true;
        return TronErrno::Success;
    }
    return TronErrno::SensorNotReady;
}

TronErrno SensorBase::pauseRecord()
{
    auto status = status_;
    if (status == SensorStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (status == SensorStatus::Inited) {
        isRecordEnabled_ = false;
        return TronErrno::Success;
    }
    return TronErrno::SensorNotReady;
}

TronErrno SensorBase::connect()
{
    auto status = status_;
    if (status == SensorStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (status == SensorStatus::Uninited) {
        TronErrno e = doConnectSensor();
        if (e == TronErrno::Success) {
            std::cout << getName() << " connect success" << std::endl;
            status_ = SensorStatus::Inited;
            return TronErrno::Success;
        } else {
            setError(e);
        }
    }
    return TronErrno::SensorAlreadyConnected;
}

TronErrno SensorBase::terminate()
{
    auto status = status_;
    if (status != SensorStatus::Terminated) {
        status_ = SensorStatus::Terminated;
        isRecordEnabled_ = false;
        isForwardEnabled_ = false;
        doDisconnectSensor();
        threadFetch_->join();
        threadStorage_->join();
        threadForward_->join();
        delete threadFetch_;
        delete threadStorage_;
        delete threadForward_;
        file_.close();
        return TronErrno::Success;
    }
    return TronErrno::SensorAlreadyClosed;
}

TronErrno SensorBase::setStorageFolder(const std::string& folder)
{
    auto status = status_;
    if (status == SensorStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (!isStoragePathSet_) {
        storagePath_ = folder + "/" + name_ + "/";
        isStoragePathSet_ = true;
        return createStorageFolder();
    }
    return TronErrno::StorageFolderAlreadySet;
}

TronErrno SensorBase::enableForward()
{
    auto status = status_;
    if (status == SensorStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (status == SensorStatus::Inited) {
        isForwardEnabled_ = true;
        return TronErrno::Success;
    }
    return TronErrno::SensorNotReady;
}
TronErrno SensorBase::disableForward()
{
    auto status = status_;
    if (status == SensorStatus::Error) {
        return TronErrno::InStatusError;
    }
    if (status == SensorStatus::Inited) {
        isForwardEnabled_ = false;
        return TronErrno::Success;
    }
    return TronErrno::SensorNotReady;
}

TronErrno SensorBase::setParameter(const std::string& type, const std::string& value)
{
    auto status = status_;
    if (status == SensorStatus::Error) {
        return TronErrno::InStatusError;
    }
    auto parameterType = SensorParameterType::_from_string_nocase_nothrow(type.c_str());
    if (!parameterType) {
        return TronErrno::InvalidParameterType;
    } else {
        parameters_[parameterType.value()] = value;
        return TronErrno::Success;
    }
}

TronErrno SensorBase::adjustParameter(const std::string& type, const std::string& value)
{
    auto status = status_;
    if (status == SensorStatus::Error) {
        return TronErrno::InStatusError;
    }
    auto parameterType = SensorParameterType::_from_string_nocase_nothrow(type.c_str());
    if (!parameterType) {
        return TronErrno::InvalidParameterType;
    } else {
        parameters_[parameterType.value()] = value;
        return doAdjustParameter(parameterType.value(), value);
    }
}

// Status
int32_t SensorBase::getId() const
{
    return id_;
}

std::string SensorBase::getName() const
{
    return name_;
}

std::string SensorBase::getStatus() const
{
    switch (status_) {
    case SensorStatus::Uninited:
        return "Uninited";
    case SensorStatus::Inited:
        return "Inited";
    case SensorStatus::Terminated:
        return "Terminated";
    default:
        return "Error";
    }
    return "Error";
}

TronErrno SensorBase::getDiagnosis() const
{
    if (latestErrno_ == TronErrno::Success) {
        if (checkNewData()) {
            return TronErrno::Success;
        } else {
            return TronErrno::NoNewData;
        }
    }
    return latestErrno_;
}

TronErrno SensorBase::setError(TronErrno e)
{
    status_ = SensorStatus::Error;
    return latestErrno_ = e;
}

// Private
TronErrno SensorBase::createStorageFolder()
{
    auto status = status_;
    if (status == SensorStatus::Error) {
        return TronErrno::InStatusError;
    }
    int ret = system(("mkdir -p '" + storagePath_ + "'").c_str());
    if (ret == 0) {
        return openNewStorageFile();
    }
    return setError(TronErrno::CanNotCreateFolder);
}

TronErrno SensorBase::openNewStorageFile()
{
    auto status = status_;
    if (status == SensorStatus::Error) {
        return TronErrno::InStatusError;
    }
    file_.close();

    std::ostringstream filename;
    filename << storagePath_;
    filename.fill('0');
    filename.width(FileNameWidth);
    filename << fileNumberCounter_++;
    filename << ".bin";
    file_.open(filename.str(), std::ios::out | std::ios::binary);

    if (file_.is_open()) {
        fileSizeCounter_ = 0;
        return TronErrno::Success;
    } else {
        setError(TronErrno::CanNotOpenFile);
    }

    return TronErrno::Success;
}

void SensorBase::fetchThreadFunc()
{
    while (true) {
        auto status = status_;
        if (status == SensorStatus::Error || status == SensorStatus::Terminated) {
            break;
        }
        if (status == SensorStatus::Inited) {
            if (auto rawdata = doFetchRawData()) {
                latestDataTimestampNs_ = rawdata->timestampReceiveNs;
                if (isRecordEnabled_) {
                    queueStorage_.push(rawdata);
                }
                if (isForwardEnabled_) {
                    queueForward_.push(rawdata);
                }
            }
        }
    }
}

void SensorBase::storageThreadFunc()
{
    while (true) {
        auto status = status_;
        if (status == SensorStatus::Error || status == SensorStatus::Terminated) {
            break;
        }
        if (!queueStorage_.empty()) {
            auto rawdata = queueStorage_.wait_and_pop();
            if ((fileSizeCounter_ != 0) && (fileSizeCounter_ + rawdata->length > FileMaxSize_)) {
                openNewStorageFile();
            }
            file_.write(reinterpret_cast<const char*>(rawdata.get()), rawdata->length);
            fileSizeCounter_ += rawdata->length;
        }
    }
}

void SensorBase::forwardThreadFunc()
{
    while (true) {
        auto status = status_;
        if (status == SensorStatus::Error || status == SensorStatus::Terminated) {
            break;
        }
        if (!queueForward_.empty()) {
            auto rawdata = queueForward_.wait_and_pop();
            auto data = doConvertData(rawdata);
            // TODO
            // Send Data via Socket
        }
    }
}

bool SensorBase::checkNewData() const
{
    return (getSystemTimestamp() - latestDataTimestampNs_ < MaxDataDurationNs_);
}

}  // namespace wayz