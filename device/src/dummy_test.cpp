#include "device/include/factory.hpp"
#include "unistd.h"

using namespace wayz::hera;

int main()
{
    log::onlyprint();

    auto storage = storage::StorageManager::open("test.hera", false);
    auto ipc_queue = ipc::IPCQueue<device::data::SensorData>::create();
    ipc_queue->open(0, ipc::OpenMode::Write, false);
    auto device0 = device::Factory::create(0, "dummy/foobar", "foobar0", true, ipc_queue.get(), storage.get());
    storage->finish_add_device();

    device0->parameter("DummyRate", "1");
    device0->parameter("DummyValue", "1");
    device0->parameter("d", "1123.223123");
    device0->parameter("i", "1123123");
    device0->parameter("save_format", "RED");
    device0->parameter("IpAddress", "10.114.514.1919");
    log::info << 'A' << log::endl;
    log::info << device0->get_parameters_json() << log::endl;
    log::info << "Stopping" << log::endl;
}