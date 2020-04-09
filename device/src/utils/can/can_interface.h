/************************************************************************
* Project        Nebula Platform
* (c) copyright  2018-2025
* Company        Voyager
*                All rights reserved
*************************************************************************
* file     CANInterface.h
* group    Flex Can Driver
* author   Jianchu Hu
*/

#pragma once
#include "error_code.h"

/// Maximal length of the supported CAN message id [bits].
#define CAN_ID_MAX_LEN 29

/// Maximal length of the supported CAN payload [bytes].
#define CANPACKET_MAX_LEN 64

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CAN_PACKET_RECV_NUM 10
#pragma pack(push, 1) // Makes sure you have consistent structure packings.

/// CAN packet.
typedef struct SCANPacket
{
  /// Timestamp of the message in timeval struct
  struct timeval timeStamp;

  /// CAN ID (Arbitration Id) of the message sender.
  uint32_t arbitrationId;

  /// Number of bytes of the payload.
  uint16_t dlc;

  /// Packet Payload.
  uint8_t packet[CANPACKET_MAX_LEN];
} SCANPacket;

#pragma pack(pop)

SALStatus SALCanInit(const char* confFilePath);
SALStatus SALCanDeInit();
SALStatus SALCanOpenPort(uint32_t portId);
SALStatus SALCanClosePort(uint32_t portId);
SALStatus SALCanRecvPacket(uint32_t portId, SCANPacket *packet, uint32_t *RecvNum, uint32_t ReqNum);
SALStatus SALCanSendPacket(uint32_t portId, const SCANPacket* packet, uint32_t num);

#ifdef __cplusplus
}
#endif
