//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <chrono>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "../src/sensors/lidar/sensor_lidar.hpp"

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 1;
    }
    char* folder = argv[1];

    wayz::SensorBase* lidar0 = new wayz::SensorLidar(0, "lidar0");
    lidar0->setParameter("IpAddress", "10.0.10.100");
    lidar0->setParameter("DataPort", "2368");  
    lidar0->setParameter("TelemetryPort", "8308");

    lidar0->connect();
    lidar0->setStorageFolder(folder);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    lidar0->startRecord();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    lidar0->pauseRecord();

  //  lidar0->adjustParameter("DummyValue", "1234");
    
  /*  std::cout << "Dummy0 has type: " << lidar0->getType() << std::endl;
    std::cout << "Dummy0 has id: " << lidar0->getId() << std::endl;
    std::cout << "Dummy0 has name: " << lidar0->getName() << std::endl;
    std::cout << "Dummy0 is in status: " << lidar0->getStatus() << std::endl;
    std::cout << "Dummy0 return diagnosis errno: " << lidar0->getDiagnosis() << std::endl;
    std::cout << "Dummy0 is in status: " << lidar0->getStatus() << std::endl;

    lidar0->startRecord();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    lidar0->pauseRecord();
    lidar0->terminate();*/

    delete lidar0;
    return 0;
}