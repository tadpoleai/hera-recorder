//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once
#include "../device.hpp"

namespace wayz {
namespace tron {

class FlirEmbeddedTimestamp {
public:
    explicit FlirEmbeddedTimestamp(uint32_t rawdata);
    int64_t get_us() const;
    int64_t get_ns() const;

private:
    int64_t time_in_cycle_;
};

class FlirEmbeddedShutter {
public:
    explicit FlirEmbeddedShutter(uint32_t rawdata);
    int64_t get_us() const;
    int64_t get_ns() const;

private:
    static constexpr int32_t ShutterStepUs_ = 66;
    int64_t duration_us_;
};

class CameraTimestamp final {
public:
    CameraTimestamp();
    CameraTimestamp(const CameraTimestamp&) = delete;
    CameraTimestamp& operator=(const CameraTimestamp&) = delete;

    // return, is calculation valid
    bool get_intrinsic_time(int64_t& out,
                            const int64_t time_received_ns,
                            const FlirEmbeddedTimestamp& embedded_timestamp,
                            const FlirEmbeddedShutter& embedded_shutter);

private:
    static constexpr int32_t Fps_ = 10;
    static constexpr int64_t PeriodUs_ = 1000000LL / Fps_;
    static constexpr int64_t PeriodNs_ = 1000000000LL / Fps_;
    static constexpr int64_t ShiftationUs_ = -20000;
    static constexpr int64_t ShiftationToleranceUs_ = 5000;
    static constexpr int64_t EmbeddedPeriodUs_ = 128000000LL;  // 128 seconds;
    static constexpr int64_t SyncPeriodNs_ = 3 * 1000000000LL;

    static constexpr int64_t MaxMatchedIntervalUs_ =
            PeriodUs_ + ShiftationUs_ + ShiftationToleranceUs_;
    static constexpr int64_t MinMatchedIntervalUs_ =
            PeriodUs_ + ShiftationUs_ - ShiftationToleranceUs_;

private:
    bool check_match(int64_t us);

    int64_t last_embedded_timestamp_us_;  // embedded timestamp in us, ranged [0s, 128s);
    int64_t accumulated_period_us_;       // accumulated period, in us, integer multiple of
                                          // EmbeddedPeriodUs_
    int64_t last_intrinsic_time_us_;
    int64_t last_sync_time_multipler_;  // time of last aligned tick at integer multiple of
                                        // SyncPeriodS_ second, in times of SyncPeriodS_;
    int64_t frame_count_;               // frame count from last aligned tick
    bool inited;
};

}  // namespace tron
}  // namespace wayz