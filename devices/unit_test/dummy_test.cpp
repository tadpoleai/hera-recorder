#include "../src/device_factory.hpp"
#include "unistd.h"

using namespace wayz::hera;

int main()
{
    auto device0 = DeviceFactory::create(0, "dummy/foobar", "dum0", "record");
    device0->define_parameter("DummyValue", "256");
    device0->define_parameter("DummyRate", "10");
    device0->start();
    device0->record(1);
    usleep(2000000);
    device0->stop();
}