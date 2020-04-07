#pragma once

#include <stdint.h>

#define AY_SUCCESS 0
#define AY_FAILED -1

typedef enum {
    AY_EVENT_TYPE_FRAMEDONE = 0x1002,
    AY_EVENT_TYPE_MAX,
} Ay_EventType_t;

struct AppContext {
    int8_t sCsiPort;       ///< mipi CSI的接口号，目前只支持 0
    uint32_t mWidth;       ///< sensor图像宽度 目前只支持 1280
    uint32_t mHeight;      ///< sensor图像高度  目前只支持 800
                           /// ** event counters and flags **
    bool mError;           ///< to signal ISP problems
    uint32_t mFrmDoneCnt;  ///< number of frames done events
    uint32_t mFrmCnt;      ///< number of frames grabbed by the app
};

typedef struct FrameTimeStamp {
    uint64_t mStartTime;     ///< in Linux jiffies in us
    uint64_t mLastDoneTime;  ///< in Linux jiffies in us
} FrameTimeS;

/**
 * \Func: VisionSdk_Initialize
 * 初始化函数,调用此函数前务必对传入的参数进行初始化
 * \param AppContext[in]
 * \return 成功返回AY_SUCCESS ，失败返回AY_FAILED
 **/
int32_t VisionSdk_Initialize(AppContext& arContext);
/**
 * \func: VisionSdk_Release
 *  资源释放
 * \param AppContext[in]
 * \return 成功返回AY_SUCCESS ，失败返回AY_FAILED
 **/
int32_t VisionSdk_Release(AppContext& arContext);
/**
 * \Func: VisionSdk_LibsPrepare
 *   准备资源和配置，以及注册回调函数
 * \param AppContext[in]
 * \param apSeqEventCallBack [in] 回调函数
 * \return 成功返回AY_SUCCESS ，失败返回AY_FAILED
 **/
int32_t VisionSdk_LibsPrepare(AppContext& arContext, void (*apSeqEventCallBack)(uint32_t, void*));
/**
 * \Func: VisionSdk_ReadFrame
 *  获取一帧数据
 * \param AppContext[in]
 * \param data[out]  一帧yuv数据， 必须分配空间
 * \param size[out]  一帧yuv数据长度
 * \return 成功返回AY_SUCCESS ，失败返回AY_FAILED
 **/
int32_t VisionSdk_ReadFrame(AppContext& arContext, uint8_t** data, uint64_t* size);
/**
 * \Func: VisionSdk_Display
 * 将一帧数据放到framebuffer，通过hdmi显示实时画面
 * \param  AppContext[in]
 * \return 成功返回AY_SUCCESS ，失败返回AY_FAILED
 **/
int32_t VisionSdk_Display(AppContext& arContext);

int32_t VisionSdk_GetTimeStatus(FrameTimeS& frameTime);
