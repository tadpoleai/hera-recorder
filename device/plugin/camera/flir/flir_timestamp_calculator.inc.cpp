///
/// @file flir_timestamp_calculator.inc.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class FlirTimestampCalculator and its helper classes
/// @version 0.1
/// @date 2019-11-08
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "flir_timestamp_calculator.hpp"

#include <cmath>

#include "common/include/logger/logger.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace camera {
namespace flir {

/// Extract second_count, cycle_count and cycle offset from corresponding bits,
/// and sum them with defined multiplier
FlirEmbeddedTimestamp::FlirEmbeddedTimestamp(uint32_t rawdata)
{
    uint32_t second_count = rawdata >> (13 + 12);
    uint32_t cycle_count = 0x1FFF & (rawdata >> (12));
    uint32_t cycle_offset = 0xFFF & rawdata;

    // Calculate embedded timestamp in us
    time_in_cycle_us_ = second_count * 1000000 + cycle_count * (1000000 / 8000) + cycle_offset / 25;
}

int64_t FlirEmbeddedTimestamp::get_us() const
{
    return time_in_cycle_us_;
}

int64_t FlirEmbeddedTimestamp::get_ns() const
{
    return time_in_cycle_us_ * 1000;
}

/// Check if embbed meta contains shutter info first,
/// and then extract it and multiple by ShutterStepUs_
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

FlirTimestampCalculator::FlirTimestampCalculator() :
    last_embedded_timestamp_us_(0),
    accumulated_cycle_us_(0),
    last_intrinsic_time_us_(0),
    last_sync_time_multipler_(0),
    frame_count_(0),
    inited(false)
{}

bool FlirTimestampCalculator::get_intrinsic_time(int64_t& out_ns,
                                                 const int64_t time_received_ns,
                                                 const FlirEmbeddedTimestamp& embedded_timestamp,
                                                 const FlirEmbeddedShutter& embedded_shutter)
{
    if (last_embedded_timestamp_us_ > embedded_timestamp.get_us()) {
        accumulated_cycle_us_ += EmbeddedTimestampCycleUs_;
    }
    last_embedded_timestamp_us_ = embedded_timestamp.get_us();

    int64_t intrinsic_time_us_ = accumulated_cycle_us_ + embedded_timestamp.get_us() - embedded_shutter.get_us();
    int64_t escaped_time_us = intrinsic_time_us_ - last_intrinsic_time_us_;
    last_intrinsic_time_us_ = intrinsic_time_us_;

    if (check_match(escaped_time_us)) {
        last_sync_time_multipler_ = time_received_ns / SyncCycleNs_;
        frame_count_ = 1;
        if (!inited) {
            log::info << "FlirTs:: check match succeed, synced" << log::endl;
        }
        inited = true;
    } else {
        frame_count_ += std::round(escaped_time_us / float(PeriodUs_));
    }

    bool result;
    result = inited;
    if (frame_count_ > SyncCycleNs_ / PeriodNs_) {
        if (inited) {
            log::warn << "FlirTs:: frame count out of range, maybe sync is lost" << log::endl;
        }
    }

    out_ns = last_sync_time_multipler_ * SyncCycleNs_ + frame_count_ * PeriodUs_ * 1000LL;
    if (frame_count_ == 1) {
        out_ns += ShiftationUs_ * 1000LL;
    }
    out_ns += embedded_shutter.get_ns() / 2;

    return result;
}

bool FlirTimestampCalculator::check_match(int64_t interval_us)
{
    return (interval_us < MaxMatchedIntervalUs_ && interval_us > MinMatchedIntervalUs_);
}

}  // namespace flir
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz