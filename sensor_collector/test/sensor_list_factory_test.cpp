//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "../src/sensor_list_factory.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 1;
    }
    char* storage_folder = argv[1];

    wayz::SensorListFactory sensors;

    std::vector<wayz::SensorConstructor> sensor_constructors({
        {"dummy1", "dummy", {{0, "8"}, {1, "0x11223344"}}},
        {"dummy2", "dummy", {{0, "4"}, {1, "0x19144511"}}}
    });

    sensors.create_sensors(sensor_constructors);
    std::cout << "Sensors created" << std::endl;

    sensors.initialize();
    std::cout << "Sensors initialzed" << std::endl;

    sensors.set_storage_folder(storage_folder);
    std::cout << "Storage folder set as" << storage_folder << std::endl;

    sensors.start_saving();
    std::cout << "Start saving" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    sensors.set_sensor_parameters(1, {{1, "0x01020304"}});
    std::cout << "Set parameters" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    sensors.pause_saving();
    std::cout << "Pause saving" << std::endl;

    sensors.terminate();
    std::cout << "Terminated" << std::endl;
    return 0;
}