//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "../src/sensors/dummy/sensor_dummy.hpp"

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 1;
    }
    char* folder = argv[1];

    wayz::SensorBase* dummy0 = new wayz::SensorDummy(0, "dummy0");
    dummy0->setParameter("DummyRate", "1");
    dummy0->setParameter("DummyValue", "2019");
    
    dummy0->connect();
    dummy0->setStorageFolder(folder);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    dummy0->startRecord();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    dummy0->pauseRecord();

    dummy0->adjustParameter("DummyValue", "1234");
    
    std::cout << "Dummy0 has type: " << dummy0->getType() << std::endl;
    std::cout << "Dummy0 has id: " << dummy0->getId() << std::endl;
    std::cout << "Dummy0 has name: " << dummy0->getName() << std::endl;
    std::cout << "Dummy0 is in status: " << dummy0->getStatus() << std::endl;
    std::cout << "Dummy0 return diagnosis errno: " << dummy0->getDiagnosis() << std::endl;
    std::cout << "Dummy0 is in status: " << dummy0->getStatus() << std::endl;

    dummy0->startRecord();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    dummy0->pauseRecord();
    dummy0->terminate();

    delete dummy0;
    return 0;
}