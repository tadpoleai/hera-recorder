#include <atomic>
#include <iostream>
#include <string>

#include "common/include/third_party/json.hpp"
#include "device/include/factory.hpp"
#include "unistd.h"

using namespace wayz::hera;
using nlohmann::json;

std::atomic<bool> g_stop;

void sig_int_handler_func(int s)
{
    g_stop = true;
    log::info << "HeraMain: Sigint Received, Stopping" << log::endl;
}

int main(int argc, char** argv)
{
    log::onlyprint();

    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = sig_int_handler_func;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    std::string profile = "config.json";
    if (argc >= 2) {
        profile = argv[1];
    } else {
        log::error << "Usage: " << argv[0] << " <config.json>" << log::endl;
        log::flush();
        exit(-1);
    }

    json profile_json;
    try {
        std::ifstream ifs;
        ifs.open(profile, std::ios::in);
        ifs >> profile_json;
        ifs.close();
    } catch (std::exception& e) {
        log::error << "Can not open config file" << profile << ", since " << e.what() << log::endl;
        log::flush();
        exit(-1);
    }

    std::string storage_name;
    try {
        auto now = time::Timestamp::now();
        storage_name = now.to_datetime() + "_" + profile_json["storage"].get<std::string>();
        auto storage = storage::StorageManager::open(storage_name, false);
        log::info << "Openned Storage '" << storage_name << "'" << log::endl;

        std::vector<std::unique_ptr<ipc::IPCQueue<device::data::SensorData>>> ipc_queues;
        for (const auto& ipc : profile_json["ipcs"]) {
            auto ipc_queue = ipc::IPCQueue<device::data::SensorData>::create();
            ipc_queue->open(ipc["key"], ipc::OpenMode::Write, true, ipc["length"], ipc["size"]);
            log::info << "Create IPC, ipc key: " << ipc["key"] << " ipc length: " << ipc["length"]
                      << " ipc size: " << ipc["size"] << log::endl;
            ipc_queues.emplace_back(std::move(ipc_queue));
        }

        uint32_t id = 0;
        std::vector<device::DevicePtr> devices;
        for (const auto& device : profile_json["devices"]) {
            std::string type = device["type"];
            std::string name = device["name"];
            int32_t forward_index = device["forward"];
            bool forward_bool = false;
            ipc::IPCQueue<device::data::SensorData>* ipc_ptr = nullptr;

            log::info << "Adding device " << type << "/" << name << log::endl;

            if (forward_index >= 0 && forward_index < (int32_t)ipc_queues.size()) {
                ipc_ptr = ipc_queues[forward_index].get();
                forward_bool = true;

                log::info << "Registering ipc " << forward_index << " to device " << type << "/" << name << log::endl;
            }

            devices.emplace_back(device::Factory::create(id++, type, name, forward_bool, ipc_ptr, storage.get()));

            for (const auto& parameter : device["parameters"]) {
                std::string type = parameter["type"];
                std::string value = parameter["value"];
                log::info << "  Setting parameter " << type << " = " << value << log::endl;
                devices.back()->parameter(type, value);
            }
        }
        storage->finish_add_device();

        for (const auto& device : devices) {
            log::info << "Starting device " << device->get_vendor_type() << "/" << device->get_name() << log::endl;
            auto err = device->start();
            if (err) {
                log::error << device->get_name() << " Error:" << device->get_errno() << " | " << device->get_reason()
                           << log::endl;
                log::flush();
                exit(-1);
            }
        }
        log::info << "All devices started" << log::endl;

        if (int(profile_json["record"])) {
            log::info << "Start Recording" << log::endl;
            for (const auto& device : devices) {
                device->record(true);
            }
        } else {
            log::info << "Not Recording" << log::endl;
        }

        while (!g_stop)
            ;

        log::info << "Exiting" << log::endl;
        for (const auto& device : devices) {
            device->stop();
        }

        storage.reset();
        log::flush();
        exit(0);
    } catch (std::exception& e) {
        log::error << "Error: " << e.what() << log::endl;
        log::flush();
        exit(-1);
    }
}