//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "sensor_dummy.hpp"

#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>

namespace wayz {

SensorDummy::SensorDummy(int32_t id, const std::string& name) : SensorBase(id, name) {}

SensorDummy::~SensorDummy() {}

SensorType SensorDummy::getType() const
{
    return SensorType::Dummy;
}

TronErrno SensorDummy::doConnectSensor()
{
    if (parameters_.count(SensorParameterType::DummyValue)) {
        value_ = std::stoi(parameters_[SensorParameterType::DummyValue]);
    } else {
        return setError(TronErrno::InsufficientParameters);
    }

    if (parameters_.count(SensorParameterType::DummyRate)) {
        periodMs_ = 1000 / std::stof(parameters_[SensorParameterType::DummyRate]);
    } else {
        return setError(TronErrno::InsufficientParameters);
    }
    return TronErrno::Success;
}

void SensorDummy::doDisconnectSensor()
{
    return;
}

std::shared_ptr<SensorRawData> SensorDummy::doFetchRawData()
{
    // Some Sensors Blocks, Simulate that
    std::this_thread::sleep_for(std::chrono::milliseconds(periodMs_));

    // Get Rawdata from a Real Sensor
    // Get Length of Rawdata First
    int32_t receivedRawdataLength = sizeof(int32_t);

    // Create a Buff to Store Rawdata
    int32_t totalLength = sizeof(SensorRawData) + receivedRawdataLength;
    SensorRawData* data = reinterpret_cast<SensorRawData*>(new uint8_t[totalLength]);

    // Fullfil Metadata (Header) of Rawdata;
    data->length = totalLength;
    data->sensorType = SensorType::Dummy;
    data->sensorDataType = SensorDataType::Dummy;
    data->sequence = sequence_++;
    data->timestampReceiveNs = getSystemTimestamp();

    // Assume Rawdata is in sensorRawataSrc
    int32_t valueToFill = value_;
    uint8_t* sensorRawataSrc = reinterpret_cast<uint8_t*>(&valueToFill);

    // Use Memcpy to fill Buff
    memcpy(reinterpret_cast<uint8_t*>(data->rawdataBuf), sensorRawataSrc, receivedRawdataLength);

    // Return a Shared Ptr
    return std::shared_ptr<SensorRawData>(data);
}

std::shared_ptr<SensorData> SensorDummy::doConvertData(
        const std::shared_ptr<SensorRawData>& rawdata)
{
    // Create a Buff to Store Data
    int32_t totalLength = sizeof(SensorData) + sizeof(DataDummy);
    SensorData* data = reinterpret_cast<SensorData*>(new uint8_t[totalLength]);

    // Fullfil Metadata (Header) of Data;
    data->length = totalLength;
    data->sensorType = rawdata->sensorType;
    data->sensorDataType = rawdata->sensorDataType;
    data->sequence = rawdata->sequence;
    data->timestampReceiveNs = rawdata->timestampReceiveNs;

    // A Pointer to Real Data
    DataDummy* dataDummyBuf = reinterpret_cast<DataDummy*>(data->dataBuf);

    // Parse Rawdata
    int32_t valueOfDummySensor = *(reinterpret_cast<int32_t*>(rawdata->rawdataBuf));

    // Fullfil Real Data
    dataDummyBuf->DummyInt = valueOfDummySensor;
    dataDummyBuf->DummyFloat = valueOfDummySensor;
    dataDummyBuf->DummyCharArray[0] = 'W';
    dataDummyBuf->DummyCharArray[1] = 'A';
    dataDummyBuf->DummyCharArray[2] = 'Y';
    dataDummyBuf->DummyCharArray[3] = 'Z';

    return std::shared_ptr<SensorData>(data);
}

TronErrno SensorDummy::doAdjustParameter(SensorParameterType type, const std::string& value)
{
    switch (type) {
    case SensorParameterType::DummyRate:
        periodMs_ = 1000 / std::stof(value);
        break;
    case SensorParameterType::DummyValue:
        value_ = std::stoi(parameters_[SensorParameterType::DummyValue]);
        break;
    default:
        return TronErrno::UnimplementedParameter;
    }
    return TronErrno::Success;
}

}  // namespace wayz