#pragma once

#include <stdint.h>


#define AY_SUCCESS 0
#define AY_FAILED -1

typedef enum {
    AY_EVENT_TYPE_FRAMEDONE = 0x1002,
    AY_EVENT_TYPE_MAX,
} Ay_EventType_t;

struct AppContext {
    int8_t sCsiPort;       // mipi CSI的接口号，目前只支持 0
    uint32_t mWidth;       // sensor图像宽度 目前只支持 1280
    uint32_t mHeight;      // sensor图像高度  目前只支持 800
                           // ** event counters and flags **
    bool mError;           ///< to signal ISP problems
    uint32_t mFrmDoneCnt;  ///< number of frames done events
    uint32_t mFrmCnt;      ///< number of frames grabbed by the app
};

typedef struct frameSyncTime {
    uint64_t mFrmDoneCnt;
    int64_t mStartTime;      //第一帧数据开始的时间
    int64_t mLastDoneTime;   //帧处理完成的时间
    int64_t mFrameSyncTime;  // FRAMESYNC 发出的时间
    int64_t delta_ns;        // framesync 发生的时间与帧处理完成的时间差。
} FrameSyncTime;

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
int32_t VisionSdk_ReadFrame(AppContext& arContext, uint8_t** data, uint64_t* size, uint32_t msTimeout);

/**
 * \Func: VisionSdk_Display
 * 通过hdmi显示实时画面
 * \param
 * \return
 **/
void VisionSdk_Display(void);
/**
 * \Func: VisionSDK_FramePush
 * 将一帧数据放到framebuffer
 * \param  AppContext[in]
 * \return 成功返回AY_SUCCESS ，失败返回AY_FAILED
 **/
int32_t VisionSDK_FramePush(AppContext& arContext);
/**
 * \Func: VisionSdk_GetFrameSyncTimeStatus
 *  获取一帧数据的详细时间戳
 * \param mTimeStatus[out]
 * \return 成功返回AY_SUCCESS ，失败返回AY_FAILED
 **/
int32_t VisionSdk_GetFrameSyncTimeStatus(FrameSyncTime& mTimeStatus);
