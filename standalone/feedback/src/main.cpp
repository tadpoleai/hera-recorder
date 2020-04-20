#include <hera/device/include.hpp>

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

    auto ipc_feedback_ = ipc::IPCQueue<device::data::SensorData>::create();
    ipc_feedback_->open(device::data::LocalizationResult::IPCKeyS32VGeely,
                        ipc::OpenMode::Write,
                        false,
                        device::data::LocalizationResult::IPCNumElement,
                        device::data::LocalizationResult::IPCElementSize);

    uint32_t sequence = 0;
    while (!g_stop) {
        auto data = device::data::SensorData::create_direct(device::SensorDataType::OdometryLocalizationResult,
                                                            sizeof(device::data::LocalizationResult),
                                                            0,
                                                            sequence++);
        auto location_result = reinterpret_cast<device::data::LocalizationResult*>(data.get());
        location_result->position[0] = 123;
        location_result->position[1] = -256;
        location_result->position[2] = 5.5;

        location_result->orientation[0] = 12;
        location_result->orientation[1] = 34;
        location_result->orientation[2] = 56;

        ipc_feedback_->write(data);
        usleep(200000);
    }

    ipc_feedback_->close();

    log::info << "Stopping" << log::endl;
}