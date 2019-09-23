//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "../src/devices/dummy/dummy.hpp"

using namespace wayz;

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 1;
    }
    char* folder = argv[1];

    tron::Device* device0 = new tron::Dummy(0, "dummy0");
    device0->set_parameter("DummyRate", "1");
    device0->set_parameter("DummyValue", "2019");
    
    device0->start();
    device0->set_storage(folder);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    device0->start_record();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    device0->pause_record();

    device0->adjust_parameter("DummyValue", "1234");
    std::cout << "Device0 has type: " << device0->get_type() << std::endl;
    std::cout << "Device0 has id: " << device0->get_id() << std::endl;
    std::cout << "Device0 has name: " << device0->get_name() << std::endl;
    std::cout << "Device0 is in status: " << device0->get_status() << std::endl;
    std::cout << "Device0 return diagnosis errno: " << device0->get_errno() << std::endl;
    std::cout << "Device0 has storaged bytes: " << device0->get_volume() << std::endl;

    device0->stop();
    
    delete device0;
    return 0;
}