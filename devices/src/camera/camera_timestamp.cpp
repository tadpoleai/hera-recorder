//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "camera_timestamp.hpp"

#include <cmath>

namespace wayz {
namespace tron {

FlirEmbeddedTimestamp::FlirEmbeddedTimestamp(uint32_t rawdata)
{
    uint32_t second_count = rawdata >> (13 + 12);
    uint32_t cycle_count = 0x1FFF & (rawdata >> (12));
    uint32_t cycle_offset = 0xFFF & rawdata;

    // Calculate embedded timestamp in us
    time_in_cycle_ = second_count * 1000000 + cycle_count * (1000000 / 8000) + cycle_offset / 25;
}
int64_t FlirEmbeddedTimestamp::get_us() const
{
    return time_in_cycle_;
}
int64_t FlirEmbeddedTimestamp::get_ns() const
{
    return time_in_cycle_ * 1000;
}

FlirEmbeddedShutter::FlirEmbeddedShutter(uint32_t rawdata)
{
    uint32_t embbed_shutter_steps = 0;
    if (rawdata > 0x70000000) {
        embbed_shutter_steps = rawdata & 0x000FFFFF;
    }
    duration_us_ = embbed_shutter_steps * ShutterStepUs_;
}
int64_t FlirEmbeddedShutter::get_us() const
{
    return duration_us_;
}
int64_t FlirEmbeddedShutter::get_ns() const
{
    return duration_us_ * 1000;
}

CameraTimestamp::CameraTimestamp() :
    last_embedded_timestamp_us_(0),
    accumulated_period_us_(0),
    last_intrinsic_time_us_(0),
    last_sync_time_multipler_(0),
    frame_count_(0),
    inited(false)
{}

bool CameraTimestamp::get_intrinsic_time(int64_t& out_ns,
                                         const int64_t time_received_ns,
                                         const FlirEmbeddedTimestamp& embedded_timestamp,
                                         const FlirEmbeddedShutter& embedded_shutter)
{
    if (last_embedded_timestamp_us_ > embedded_timestamp.get_us()) {
        accumulated_period_us_ += EmbeddedPeriodUs_;
    }
    last_embedded_timestamp_us_ = embedded_timestamp.get_us();

    int64_t intrinsic_time_us_ =
            accumulated_period_us_ + embedded_timestamp.get_us() - embedded_shutter.get_us();
    int64_t escaped_time_us = intrinsic_time_us_ - last_intrinsic_time_us_;
    last_intrinsic_time_us_ = intrinsic_time_us_;

    if (check_match(escaped_time_us)) {
        last_sync_time_multipler_ = time_received_ns / SyncPeriodNs_;
        frame_count_ = 1;
        inited = true;
    } else {
        frame_count_ += std::round(escaped_time_us / float(PeriodUs_));
    }

    bool result;
    result = inited;
    if (frame_count_ > SyncPeriodNs_ / PeriodNs_) {
        result = false;
    }

    out_ns = last_sync_time_multipler_ * SyncPeriodNs_ + frame_count_ * PeriodUs_ * 1000LL;
    if (frame_count_ == 1) {
        out_ns += ShiftationUs_ * 1000LL;
    }
    out_ns += embedded_shutter.get_ns() / 2;

    return result;
}

bool CameraTimestamp::check_match(int64_t interval_us)
{
    return (interval_us < MaxMatchedIntervalUs_ && interval_us > MinMatchedIntervalUs_);
}


}  // namespace tron
}  // namespace wayz