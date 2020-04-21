#include <atomic>

#include "device/include/device_factory.hpp"
#include "unistd.h"

using namespace wayz::hera;

std::atomic<bool> g_stop;

void sig_int_handler_func(int s)
{
    g_stop = true;
    log::info << "HeraMain: Sigint Received, Stopping" << log::endl;
}

int main()
{
    log::onlyprint();
    g_stop = false;

    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = sig_int_handler_func;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    auto storage = storage::StorageManager::open("s32vgeely.hera", false);
    auto ipc_queue = ipc::IPCQueue<device::data::SensorData>::create();
    ipc_queue->open(0, ipc::OpenMode::Write, false);
    auto device0 =
            device::DeviceFactory::create(0, "odometry/s32vgeely", "s32vgeely0", true, ipc_queue.get(), storage.get());
    storage->finish_add_device();

    device0->parameter("DataPort", "0");
    auto err = device0->start();

    if (err != HeraErrno::OK) {
        log::error << "Can not start device due to: err = " << err << ", reason = " << device0->get_reason()
                   << log::endl;
    }

    device0->record(true);

    while (!g_stop)
        usleep(10000);

    log::info << "Stopping Device0" << log::endl;
    device0->stop();

    log::info << "Stopping" << log::endl;
    storage.reset();
}