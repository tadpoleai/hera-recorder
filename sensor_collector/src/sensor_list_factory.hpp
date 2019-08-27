//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#ifndef __sensor_list_factory_hpp__
#define __sensor_list_factory_hpp__

#include <string>
#include <vector>

#include <common/data_message/sensor_and_data_types.hpp>

#include "sensors/dummy/sensor_dummy.hpp"
#include "sensors/sensor_base.hpp"

namespace wayz {

struct SensorConstructor {
    std::string sensor_name;
    std::string sensor_type_name;
    std::vector<ParamPair> sensor_parameters;
};

class SensorListFactory final {
public:
    SensorListFactory();
    SensorListFactory(const SensorListFactory&) = delete;
    SensorListFactory& operator=(const SensorListFactory&) = delete;
    ~SensorListFactory();

    // Configuration
    void create_sensors(const std::vector<SensorConstructor>& sensor_constructor);
    void set_storage_folder(const std::string& storage_folder) const;
    bool set_sensor_parameters(const uint32_t sensor_index,
                               const std::vector<ParamPair>& sensor_parameters) const;

    // State
    std::vector<bool> get_sensors_alive();

    // Control Command
    void initialize() const;
    void start_saving() const;
    void pause_saving() const;
    void terminate();

private:
    std::vector<SensorBase*> sensor_list_;
    bool terminated_;
};

}  // namespace wayz
#endif