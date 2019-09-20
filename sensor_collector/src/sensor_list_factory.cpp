//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "sensor_list_factory.hpp"

#include <string>
#include <vector>

namespace wayz {

SensorListFactory::SensorListFactory() : terminated_(false) {}

SensorListFactory::~SensorListFactory()
{
    if (!terminated_) {
        terminate();
    }
}

void SensorListFactory::create_sensors(const std::vector<SensorConstructor>& sensor_constructors)
{
    for (auto sensor_constructor : sensor_constructors) {
        auto sensor_type = SensorType::_from_string(sensor_constructor.sensor_type_name.c_str());
        auto sensor_name = sensor_constructor.sensor_name;
        SensorBase* sensor_ptr;
        switch (sensor_type) {
        case SensorType::dummy:
            sensor_ptr = new SensorDummy(sensor_name);
            break;
        /*case SensorType::imu:
            sensor_ptr = new SensorImu(sensor_name);
            break;
        case SensorType::gps:
            sensor_ptr = new SensorGps(sensor_name);
            break;
        case SensorType::camera:
            sensor_ptr = new SensorCamera(sensor_name);
            break;*/
        case SensorType::lidar:
            sensor_ptr = new SensorLidar(sensor_name);
            break;
        default:
            sensor_ptr = new SensorDummy(sensor_name);
            break;
        }
        sensor_ptr->set_sensor_parameters(sensor_constructor.sensor_parameters);
        sensor_list_.push_back(sensor_ptr);
    }
}


void SensorListFactory::set_storage_folder(const std::string& storage_folder) const
{
    for (auto sensor : sensor_list_) {
        sensor->set_storage_folder(storage_folder);
    }
}

bool SensorListFactory::set_sensor_parameters(const uint32_t sensor_index,
                                              const std::vector<ParamPair>& sensor_parameters) const
{
    try {
        return (sensor_list_.at(sensor_index))->set_sensor_parameters(sensor_parameters);
    } catch (const std::out_of_range& error) {
        return false;
    }
}

std::vector<bool> SensorListFactory::get_sensors_alive()
{
    std::vector<bool> value;
    for (auto sensor : sensor_list_) {
        value.push_back(sensor->get_sensor_alive());
    }
    return value;
}

void SensorListFactory::initialize() const
{
    for (auto sensor : sensor_list_) {
        sensor->connect_sensor();
    }
}

void SensorListFactory::start_saving() const
{
    for (auto sensor : sensor_list_) {
        sensor->start_saving();
    }
}

void SensorListFactory::pause_saving() const
{
    for (auto sensor : sensor_list_) {
        sensor->pause_saving();
    }
}

void SensorListFactory::terminate()
{
    for (auto sensor : sensor_list_) {
        sensor->disconnect_sensor();
    }
    terminated_ = true;
}

}  // namespace wayz