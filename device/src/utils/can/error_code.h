/************************************************************************
 * Project        Nebula Platform
 * (c) copyright  2018-2025
 * Company        Voyager
 *                All rights reserved
 *************************************************************************
 * file     ErrorCode.h
 * group    Common
 * author   Jianchu Hu
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define __ENUM_MACRO(e) e,
// clang-format off
#define __ERROR_CODES(s)              \
    s(SUCCESS)                       /*!< No error */ \
    s(INVALID_PARAMETER)              /*!< One of the given parameters is invalid */ \
    s(TIME_OUT)                      /*!< The request has timed out */ \
    s(BUSY_WAITING)                  /*!< busy waiting */ \
    s(SAL_CANNOT_INITIALIZE)         /*!< A sensor cannot be initialized, e.g., sensor might not be responding. config file parameters error */ \
    s(OP_FAILED)                     /*!< opreation faiiled */   \
    s(DEVICE_NOT_INITIALIZED)          /*!< device not initialized */   \
// clang-format on


// Status definition.

typedef enum SALStatus {
  __ERROR_CODES(__ENUM_MACRO)
} SALStatus;

#ifdef __cplusplus
}
#endif

