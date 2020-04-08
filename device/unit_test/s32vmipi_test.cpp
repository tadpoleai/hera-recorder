#include "device_factory.hpp"
#include "unistd.h"

using namespace wayz::hera;

volatile bool g_stop = false;

///
/// @brief Handler Ctrl+C(SIGINT)
///
/// @param s signal
void sig_int_handler_func(int s)
{
    g_stop = true;
    log::info << "HeraMain: Sigint Received, Stopping" << log::endl;
}

int main()
{
    log::onlyprint();

    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = sig_int_handler_func;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    auto storage = storage::StorageManager::open("/home/root/work_cc/hera/s32v_test.hera", false);
    auto ipc_queue = ipc::IPCQueue<device::data::SensorData>::create();
    ipc_queue->open(0, ipc::OpenMode::Write, false, 4UL, (1 << 20));
    auto device0 = device::DeviceFactory::create(0, "camera/s32vmipi", "camera0", true, ipc_queue.get(), storage.get());
    storage->finish_add_device();

    auto err = device0->start();
    if (err != HeraErrno::OK) {
        log::error << "Can not start device due to: err = " << err << ", reason = " << device0->get_reason()
                   << log::endl;
    }

    device0->record(true);

    while (!g_stop)
        ;

    device0->stop();

    log::info << "Stopping" << log::endl;
    storage.reset();
}