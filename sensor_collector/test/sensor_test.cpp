//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <chrono>
#include <string>
#include <utility>

#include "../src/sensors/dummy/sensor_dummy.hpp"

int main(int argc, char ** argv)
{
    if (argc != 2) {
        return 1;
    }
    char* storage_folder = argv[1];

    wayz::SensorDummy dummy1(std::string(storage_folder),
                             std::string("dummy1"));
                
    auto rate_parameter = std::make_pair<std::string, std::string>("rate", "10.0");
    auto parameters = std::vector<wayz::Dictionary>();
    parameters.push_back(rate_parameter);
    dummy1.connect_sensor(parameters);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    dummy1.start_saving();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    dummy1.pause_saving();
    dummy1.disconnect_sensor();
    
    return 0;
}