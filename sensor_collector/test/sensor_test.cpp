//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <chrono>
#include <iostream>
#include <string>
#include <utility>

#include "../src/sensors/dummy/sensor_dummy.hpp"

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 1;
    }
    char* storage_folder = argv[1];

    wayz::SensorDummy dummy1(std::string(storage_folder), std::string("dummy1"));


    auto parameter_names = dummy1.get_sensor_parameter_names();
    for (auto param : parameter_names) {
        std::cout << "ID: " << param.first << ", " << param.second << std::endl;
    }

    {
        auto rate_param_pair = std::make_pair<int32_t, std::string>(0, "4");
        auto value_param_pair = std::make_pair<int32_t, std::string>(1, "0x19144511");
        auto param_pairs = std::vector<wayz::ParamPair>();
        param_pairs.push_back(rate_param_pair);
        param_pairs.push_back(value_param_pair);
        dummy1.set_sensor_parameters(param_pairs);
    }


    dummy1.connect_sensor();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    dummy1.start_saving();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    dummy1.pause_saving();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        auto rate_param_pair = std::make_pair<int32_t, std::string>(0, "10");
        auto value_param_pair = std::make_pair<int32_t, std::string>(1, "6570806");
        auto param_pairs = std::vector<wayz::ParamPair>();
        param_pairs.push_back(rate_param_pair);
        param_pairs.push_back(value_param_pair);
        dummy1.set_sensor_parameters(param_pairs);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    dummy1.start_saving();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    dummy1.pause_saving();

    dummy1.disconnect_sensor();

    return 0;
}