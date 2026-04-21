///
/// @file plugin_entry.cpp
/// @brief Insta360 phase-1 plugin: realtime preview bitstream + gyro raw capture
///

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "plugin_common.hpp"
#include "plugin_param.hpp"

#ifdef WITH_DRIVER
#include <camera/camera.h>
#include <camera/device_discovery.h>
#include <camera/photography_settings.h>
#endif

namespace wayz {
namespace hera {
namespace device {
namespace camera {
namespace insta {

#ifdef WITH_DRIVER
namespace {

constexpr size_t kMaxVideoPayloadBytes = 8 * 1024 * 1024;
constexpr size_t kMaxGyroPayloadBytes = 2 * 1024 * 1024;
constexpr size_t kMaxQueueBytes = 64 * 1024 * 1024;

ins_camera::VideoResolution map_video_resolution(const VideoResolution value)
{
    if (value._to_integral() == static_cast<int32_t>(VideoResolution::R2560x1280P30)) {
        return ins_camera::VideoResolution::RES_2560_1280P30;
    }
    if (value._to_integral() == static_cast<int32_t>(VideoResolution::R1440x720P30)) {
        return ins_camera::VideoResolution::RES_1440_720P30;
    }
    return ins_camera::VideoResolution::RES_3840_1920P30;
}

ins_camera::VideoResolution map_lrv_resolution(const bool fixed_lrv_resolution)
{
    (void)fixed_lrv_resolution;
    return ins_camera::VideoResolution::RES_1440_720P30;
}

template <typename CameraType>
auto sync_local_time_impl(CameraType* cam, uint64_t t, int) -> decltype(cam->SyncLocalTimeToCamera(t, 0), bool())
{
    return cam->SyncLocalTimeToCamera(t, 0);
}

template <typename CameraType>
auto sync_local_time_impl(CameraType* cam, uint64_t t, long) -> decltype(cam->SyncLocalTimeToCamera(t), bool())
{
    return cam->SyncLocalTimeToCamera(t);
}

bool sync_local_time_to_camera(const std::shared_ptr<ins_camera::Camera>& cam, uint64_t t)
{
    if (!cam) {
        return false;
    }
    return sync_local_time_impl(cam.get(), t, 0);
}

}  // namespace
#endif

HERA_PLUGIN_DEFINE_START("camera/insta", 0x0421, 128)

#include "plugin_data.hpp"

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

struct RawSample {
    bool is_video;
    int64_t device_ts_ns;
    uint64_t host_ts_ns;
    uint8_t stream_type;
    int32_t stream_index;
    uint32_t sample_count;
    std::vector<uint8_t> payload;
};

struct RuntimeState {
    std::mutex mutex;
    std::condition_variable cv;
    std::deque<RawSample> queue;
    size_t queue_bytes = 0;
    bool stopped = false;

    std::atomic<uint64_t> rx_video_packets{0};
    std::atomic<uint64_t> rx_gyro_packets{0};
    std::atomic<uint64_t> out_video_packets{0};
    std::atomic<uint64_t> out_gyro_packets{0};
    std::atomic<uint64_t> drop_packets{0};
    uint64_t last_stats_log_ns = 0;
};

class HeraStreamDelegate : public ins_camera::StreamDelegate {
public:
    HeraStreamDelegate(const std::shared_ptr<RuntimeState>& runtime, bool enable_gyro) :
        runtime_(runtime), enable_gyro_(enable_gyro)
    {
    }

    void OnAudioData(const uint8_t* data, size_t size, int64_t timestamp) override
    {
        (void)data;
        (void)size;
        (void)timestamp;
    }

    void OnVideoData(const uint8_t* data,
                     size_t size,
                     int64_t timestamp,
                     uint8_t streamType,
                     int stream_index) override
    {
        if (!runtime_ || !data || size == 0) {
            return;
        }
        if (size > kMaxVideoPayloadBytes) {
            log::warn << "Insta: drop abnormal video packet, size=" << size << log::endl;
            return;
        }

        RawSample sample;
        sample.is_video = true;
        sample.device_ts_ns = timestamp;
        sample.host_ts_ns = static_cast<uint64_t>(time::Timestamp::now());
        sample.stream_type = streamType;
        sample.stream_index = stream_index;
        sample.sample_count = 0;
        sample.payload.resize(size);
        memcpy(sample.payload.data(), data, size);

        runtime_->rx_video_packets.fetch_add(1);
        enqueue(std::move(sample));
    }

    void OnGyroData(const std::vector<ins_camera::GyroData>& data) override
    {
        if (!runtime_ || !enable_gyro_ || data.empty()) {
            return;
        }

        const size_t gyro_bytes = sizeof(ins_camera::GyroData) * data.size();
        if (gyro_bytes > kMaxGyroPayloadBytes) {
            log::warn << "Insta: drop abnormal gyro packet, bytes=" << gyro_bytes << ", samples=" << data.size()
                      << log::endl;
            return;
        }

        RawSample sample;
        sample.is_video = false;
        sample.device_ts_ns = 0;
        sample.host_ts_ns = static_cast<uint64_t>(time::Timestamp::now());
        sample.stream_type = 0;
        sample.stream_index = -1;
        sample.sample_count = static_cast<uint32_t>(data.size());
        sample.payload.resize(gyro_bytes);
        memcpy(sample.payload.data(), data.data(), sample.payload.size());

        runtime_->rx_gyro_packets.fetch_add(1);
        enqueue(std::move(sample));
    }

    void OnExposureData(const ins_camera::ExposureData& data) override
    {
        (void)data;
    }

private:
    void enqueue(RawSample&& sample)
    {
        std::lock_guard<std::mutex> lock(runtime_->mutex);
        if (runtime_->stopped) {
            return;
        }

        const size_t incoming_bytes = sample.payload.size();
        if (incoming_bytes == 0 || incoming_bytes > kMaxQueueBytes) {
            runtime_->drop_packets.fetch_add(1);
            return;
        }

        while (!runtime_->queue.empty() && (runtime_->queue_bytes + incoming_bytes > kMaxQueueBytes)) {
            runtime_->queue_bytes -= runtime_->queue.front().payload.size();
            runtime_->queue.pop_front();
            runtime_->drop_packets.fetch_add(1);
        }

        runtime_->queue.emplace_back(std::move(sample));
        runtime_->queue_bytes += incoming_bytes;
        runtime_->cv.notify_one();
    }

private:
    std::shared_ptr<RuntimeState> runtime_;
    bool enable_gyro_;
};

std::shared_ptr<RuntimeState> runtime_;
std::shared_ptr<ins_camera::Camera> camera_;
std::shared_ptr<ins_camera::StreamDelegate> stream_delegate_;
std::atomic<bool> stream_started_{false};

static void maybe_log_stats(const std::shared_ptr<RuntimeState>& rt)
{
    const uint64_t now_ns = static_cast<uint64_t>(time::Timestamp::now());
    if (rt->last_stats_log_ns == 0) {
        rt->last_stats_log_ns = now_ns;
        return;
    }

    if (now_ns - rt->last_stats_log_ns < static_cast<uint64_t>(time::OneSecond)) {
        return;
    }

    rt->last_stats_log_ns = now_ns;
    log::info << "Insta: rx(video/gyro)=" << rt->rx_video_packets.load() << "/"
              << rt->rx_gyro_packets.load() << ", out(video/gyro)=" << rt->out_video_packets.load() << "/"
              << rt->out_gyro_packets.load() << ", drop=" << rt->drop_packets.load() << ", qbytes=" << rt->queue_bytes
              << log::endl;
}

#endif

HERA_PLUGIN_DEFINE_END

#ifdef WITH_DRIVER

HeraErrno DevicePlugin::connect()
{
    stream_started_.store(false);
    runtime_ = std::make_shared<RuntimeState>();

    if (local_parameters_.get_LogLevel()._to_integral() == static_cast<int32_t>(LogLevel::Verbose)) {
        ins_camera::SetLogLevel(ins_camera::LogLevel::VERBOSE);
    } else {
        ins_camera::SetLogLevel(ins_camera::LogLevel::ERR);
    }

    ins_camera::DeviceDiscovery discovery;
    auto list = discovery.GetAvailableDevices();
    if (list.empty()) {
        return handle_error(HeraErrno::CanNotOpenCamera, "Insta: no USB camera discovered");
    }

    auto selected = list[0];
    const auto sn = local_parameters_.get_DeviceSn();
    if (!sn.empty()) {
        bool found = false;
        for (const auto& item : list) {
            if (item.serial_number == sn) {
                selected = item;
                found = true;
                break;
            }
        }
        if (!found) {
            discovery.FreeDeviceDescriptors(list);
            return handle_error(HeraErrno::CanNotOpenCamera, "Insta: DeviceSn not found");
        }
    }

    camera_ = std::make_shared<ins_camera::Camera>(selected.info);
    discovery.FreeDeviceDescriptors(list);

    if (!camera_ || !camera_->Open()) {
        camera_.reset();
        return handle_error(HeraErrno::CanNotOpenCamera, "Insta: camera open failed");
    }

    camera_->SetTimeout(std::max<int32_t>(1000, local_parameters_.get_CameraTimeoutMs()));

    if (local_parameters_.get_SyncLocalTime()) {
        std::time_t now = std::time(nullptr);
        if (!sync_local_time_to_camera(camera_, static_cast<uint64_t>(now))) {
            log::warn << "Insta: SyncLocalTimeToCamera failed" << log::endl;
        }
    }

    stream_delegate_ = std::make_shared<HeraStreamDelegate>(runtime_, local_parameters_.get_EnableGyro());
    camera_->SetStreamDelegate(stream_delegate_);

    if (local_parameters_.get_EnableInCameraStitching()) {
        if (!camera_->EnableInCameraStitching(true)) {
            log::warn << "Insta: EnableInCameraStitching failed (device may not support it)" << log::endl;
        }
    }

    ins_camera::LiveStreamParam param;
    param.video_resolution = map_video_resolution(local_parameters_.get_VideoResolution());
    param.lrv_video_resulution = map_lrv_resolution(local_parameters_.get_FixedLrvResolution());
    param.video_bitrate = std::max<int32_t>(131072, local_parameters_.get_VideoBitrate());
    param.enable_audio = local_parameters_.get_EnableAudio();
    param.using_lrv = local_parameters_.get_UsingLrv();

    ins_camera::RecordParams record_params;
    record_params.resolution = param.video_resolution;
    record_params.bitrate = 0;
    if (!camera_->SetVideoCaptureParams(record_params, ins_camera::CameraFunctionMode::FUNCTION_MODE_LIVE_STREAM)) {
        log::warn << "Insta: SetVideoCaptureParams(FUNCTION_MODE_LIVE_STREAM) failed, continue with SDK defaults"
                  << log::endl;
    }

    if (!camera_->StartLiveStreaming(param)) {
        camera_->Close();
        camera_.reset();
        stream_delegate_.reset();
        runtime_.reset();
        return handle_error(HeraErrno::CanNotOpenCamera, "Insta: StartLiveStreaming failed");
    }

    stream_started_.store(true);

    return HeraErrno::Success;
}

void DevicePlugin::disconnect()
{
    auto local_rt = runtime_;
    if (local_rt) {
        std::lock_guard<std::mutex> lock(local_rt->mutex);
        local_rt->stopped = true;
        local_rt->queue.clear();
        local_rt->cv.notify_all();
    }

    if (camera_) {
        // Some CameraSDK builds may crash in StopLiveStreaming() during teardown.
        // Closing the camera session directly is stable and sufficient for cleanup.
        if (stream_started_.load()) {
            stream_started_.store(false);
        }
        camera_->Close();
    }

    stream_delegate_.reset();
    camera_.reset();
    runtime_.reset();
}

data::DeviceDataPtr DevicePlugin::fetch()
{
    auto local_rt = runtime_;
    if (!local_rt) {
        return nullptr;
    }

    RawSample sample;
    {
        std::unique_lock<std::mutex> lock(local_rt->mutex);
        const auto timeout_ms = std::max<int32_t>(1, local_parameters_.get_FetchTimeoutMs());
        if (!local_rt->cv.wait_for(lock,
                                   std::chrono::milliseconds(timeout_ms),
                                   [&]() { return local_rt->stopped || !local_rt->queue.empty(); })) {
            maybe_log_stats(local_rt);
            return nullptr;
        }

        if (local_rt->stopped || local_rt->queue.empty()) {
            return nullptr;
        }

        sample = std::move(local_rt->queue.front());
        local_rt->queue.pop_front();
        if (local_rt->queue_bytes >= sample.payload.size()) {
            local_rt->queue_bytes -= sample.payload.size();
        } else {
            local_rt->queue_bytes = 0;
        }
    }

    if (sample.is_video) {
        if (sample.payload.empty() || sample.payload.size() > kMaxVideoPayloadBytes) {
            log::warn << "Insta: skip invalid video payload size=" << sample.payload.size() << log::endl;
            return nullptr;
        }
        if (sample.payload.size() > (std::numeric_limits<uint32_t>::max() - sizeof(InstaVideoPacket))) {
            log::warn << "Insta: skip oversized video payload" << log::endl;
            return nullptr;
        }
        const auto length = static_cast<uint32_t>(sizeof(InstaVideoPacket) + sample.payload.size());
        auto data = InstaVideoPacket::create(length, id_, sequence_++);
        auto raw = static_cast<InstaVideoPacket*>(data.get());

        raw->timestamp_device_ns = sample.device_ts_ns;
        raw->timestamp_host_ns = sample.host_ts_ns;
        raw->stream_type = sample.stream_type;
        raw->stream_index = sample.stream_index;
        raw->payload_size = sample.payload.size();
        if (!sample.payload.empty()) {
            memcpy(raw->payload, sample.payload.data(), sample.payload.size());
        }

        local_rt->out_video_packets.fetch_add(1);
        maybe_log_stats(local_rt);
        return data;
    }

    if (sample.payload.empty() || sample.payload.size() > kMaxGyroPayloadBytes) {
        log::warn << "Insta: skip invalid gyro payload size=" << sample.payload.size() << log::endl;
        return nullptr;
    }
    if (sample.payload.size() > (std::numeric_limits<uint32_t>::max() - sizeof(InstaGyroPacket))) {
        log::warn << "Insta: skip oversized gyro payload" << log::endl;
        return nullptr;
    }

    const auto length = static_cast<uint32_t>(sizeof(InstaGyroPacket) + sample.payload.size());
    auto data = InstaGyroPacket::create(length, id_, sequence_++);
    auto raw = static_cast<InstaGyroPacket*>(data.get());

    raw->timestamp_host_ns = sample.host_ts_ns;
    raw->sample_count = sample.sample_count;
    raw->payload_size = sample.payload.size();
    if (!sample.payload.empty()) {
        memcpy(raw->payload, sample.payload.data(), sample.payload.size());
    }

    local_rt->out_gyro_packets.fetch_add(1);
    maybe_log_stats(local_rt);
    return data;
}

HeraErrno DevicePlugin::adjust_parameter(const std::string& type, const std::string& value)
{
    (void)type;
    (void)value;
    return HeraErrno::OK;
}

#endif

data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameters)
{
    (void)storage_data;
    (void)parameters;
    // Phase 1 only requires stable raw capture to storage.
    return data::SensorData::broken_data();
}

}  // namespace insta
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz
