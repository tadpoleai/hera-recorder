///
/// @file can_port.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Implementation of Class CANPort on Platfrom S32V
/// @date 2020-04-20
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "can_port.hpp"

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VCAN
#include "s32v_can.h"
#endif
#endif

namespace wayz {
namespace hera {
namespace device {
namespace driver {

std::mutex CANPort::mutex_;

int32_t CANPort::reference_count_ = 0;

CANPort::CANPort(const uint32_t data_port)
{
    port_ = data_port;
    port_opened_ = false;

#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VCAN
    {
        std::unique_lock<std::mutex> _(mutex_);
        if (reference_count_++ == 0) {
            if (SALCanInit("/etc/conf/drivers/can/canbus.conf") != SUCCESS) {
                return;
            }
        }
    }

    if (SALCanOpenPort(port_) == SUCCESS) {
        port_opened_ = true;
    }
#endif
#endif
}

CANPort::~CANPort()
{
#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VCAN
    SALCanClosePort(port_);

    {
        std::unique_lock<std::mutex> _(mutex_);
        if (--reference_count_ == 0) {
            // SALCanDeInit();
        }
    }
#endif
#endif
}

CANPacketPtr CANPort::read()
{
#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VCAN
    if (!port_opened_) {
        return nullptr;
    }

    SCANPacket scan_packet;
    uint32_t recv_num;

    if (SALCanRecvPacket(port_, &scan_packet, &recv_num, 1) != SUCCESS) {
        return nullptr;
    }

    if (recv_num == 0) {
        return nullptr;
    }

    auto data = CANPacket::create(scan_packet.arbitrationId, scan_packet.dlc);
    data->timestamp.tv_sec = scan_packet.timeStamp.tv_sec;
    data->timestamp.tv_nsec = scan_packet.timeStamp.tv_usec * 1000ULL;
    memcpy(data->data, scan_packet.packet, scan_packet.dlc);

    return data;
#endif
#endif
    return nullptr;
}

bool CANPort::write(const CANPacketPtr& data)
{
#ifdef WITH_DRIVER
#ifdef DEVICE_DRIVER_S32VCAN
    if (!port_opened_) {
        return false;
    }

    SCANPacket scan_packet;
    scan_packet.arbitrationId = data->id;
    scan_packet.dlc = data->dlc;
    scan_packet.timeStamp.tv_sec = data->timestamp.tv_sec;
    scan_packet.timeStamp.tv_usec = data->timestamp.tv_nsec / 1000ULL;
    memcpy(scan_packet.packet, data->data, scan_packet.dlc);

    return (SALCanSendPacket(port_, &scan_packet, 1) == SUCCESS);
#endif
#endif
    return false;
}

}  // namespace driver
}  // namespace device
}  // namespace hera
}  // namespace wayz