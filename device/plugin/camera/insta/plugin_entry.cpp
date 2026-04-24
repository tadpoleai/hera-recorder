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
#include <cstdlib>
#include <cctype>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

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

std::string join_path(const std::string& dir, const std::string& name)
{
    if (dir.empty() || dir == ".") {
        return name;
    }
    if (dir.back() == '/' || dir.back() == '\\') {
        return dir + name;
    }
    return dir + "/" + name;
}

std::string basename_from_url(const std::string& url)
{
    const auto slash = url.find_last_of("/\\");
    if (slash == std::string::npos || slash + 1 >= url.size()) {
        return "insta_record.mp4";
    }
    return url.substr(slash + 1);
}

bool download_media_urls(const std::shared_ptr<ins_camera::Camera>& camera,
                        const ins_camera::MediaUrl& media_url,
                        const std::string& output_dir)
{
    if (!camera || media_url.Empty()) {
        return false;
    }

    if (access(output_dir.c_str(), F_OK) != 0) {
        log::warn << "Insta: download directory does not exist: " << output_dir << log::endl;
        return false;
    }

    bool all_ok = true;
    const auto& urls = media_url.OriginUrls();
    for (size_t i = 0; i < urls.size(); ++i) {
        const auto& remote = urls[i];
        const auto local = join_path(output_dir, basename_from_url(remote));
        if (!camera->DownloadCameraFile(remote, local)) {
            all_ok = false;
            log::warn << "Insta: download failed: " << remote << " -> " << local << log::endl;
            continue;
        }
        log::info << "Insta: downloaded " << local << log::endl;
    }

    return all_ok;
}

bool parse_bool(const std::string& value, const bool default_value)
{
    if (value.empty()) {
        return default_value;
    }

    std::string normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        return false;
    }
    return default_value;
}

bool stop_recording_with_optional_download(const std::shared_ptr<ins_camera::Camera>& camera,
                                           const bool auto_download,
                                           const std::string& download_dir)
{
    if (!camera) {
        return false;
    }

    const auto media_url = camera->StopRecording();
    if (media_url.Empty()) {
        log::warn << "Insta: StopRecording returned empty media url" << log::endl;
        return false;
    }

    if (!auto_download) {
        for (const auto& url : media_url.OriginUrls()) {
            log::info << "Insta: recorded media url " << url << log::endl;
        }
        return true;
    }

    return download_media_urls(camera, media_url, download_dir);
}

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

bool start_recording_normal_video(const std::shared_ptr<ins_camera::Camera>& camera,
                                  const VideoResolution resolution,
                                  const int32_t video_bitrate)
{
    if (!camera) {
        return false;
    }

    ins_camera::RecordParams record_params;
    record_params.resolution = map_video_resolution(resolution);
    record_params.bitrate = std::max<int32_t>(131072, video_bitrate);

    if (!camera->SetVideoCaptureParams(record_params, ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO)) {
        log::warn << "Insta: SetVideoCaptureParams(FUNCTION_MODE_NORMAL_VIDEO) failed" << log::endl;
    }

    return camera->StartRecording();
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
std::atomic<bool> is_stream_mode_{true};
std::atomic<bool> record_started_{false};
std::atomic<bool> runtime_auto_download_{true};
std::string runtime_download_dir_{"."};
std::mutex camera_op_mutex_;

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
    record_started_.store(false);
    runtime_ = std::make_shared<RuntimeState>();

    if (local_parameters_.get_LogLevel()._to_integral() == static_cast<int32_t>(LogLevel::Verbose)) {
        ins_camera::SetLogLevel(ins_camera::LogLevel::VERBOSE);
    } else {
        ins_camera::SetLogLevel(ins_camera::LogLevel::ERR);
    }

    ins_camera::DeviceDiscovery discovery;
    std::vector<ins_camera::DeviceDescriptor> list;
    const auto discover_timeout_ms = std::max<int32_t>(1000, local_parameters_.get_CameraTimeoutMs());
    const auto discover_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(discover_timeout_ms);
    while (std::chrono::steady_clock::now() < discover_deadline) {
        list = discovery.GetAvailableDevices();
        if (!list.empty()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (list.empty()) {
        return handle_error(HeraErrno::CanNotOpenCamera,
                            "Insta: no USB camera discovered (check USB mode=Android and retry)");
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

    // After an abnormal session end (e.g. SIGKILL, daemon crash), the camera's USB bulk-IN
    // endpoint retains buffered bytes from the previous protocol exchange. The first Open()
    // call reads those stale bytes, fails to parse the response packet ("parse packet error"
    // in io_device.cpp), and returns false — even though the camera is physically connected
    // and healthy. A second Open() call on the same device descriptor starts with a clean
    // pipe and always succeeds. We implement that retry here so callers never see the error.
    static constexpr int kOpenRetries = 2;
    for (int attempt = 1; attempt <= kOpenRetries; ++attempt) {
        camera_ = std::make_shared<ins_camera::Camera>(selected.info);
        bool opened = false;
        try {
            opened = camera_ && camera_->Open();
        } catch (const std::exception& e) {
            if (attempt < kOpenRetries) {
                log::warn << "Insta: camera open exception on attempt " << attempt << ": " << e.what()
                          << " — retrying" << log::endl;
                camera_.reset();
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                continue;
            }
            discovery.FreeDeviceDescriptors(list);
            camera_.reset();
            return handle_error(HeraErrno::CanNotOpenCamera, std::string("Insta: camera open exception: ") + e.what());
        } catch (...) {
            if (attempt < kOpenRetries) {
                log::warn << "Insta: camera open unknown exception on attempt " << attempt << " — retrying" << log::endl;
                camera_.reset();
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                continue;
            }
            discovery.FreeDeviceDescriptors(list);
            camera_.reset();
            return handle_error(HeraErrno::CanNotOpenCamera, "Insta: camera open unknown exception");
        }

        if (opened) {
            break;  // success
        }
        if (attempt < kOpenRetries) {
            log::warn << "Insta: camera->Open() failed on attempt " << attempt << " (stale USB pipe) — retrying"
                      << log::endl;
            camera_.reset();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        } else {
            discovery.FreeDeviceDescriptors(list);
            camera_.reset();
            return handle_error(HeraErrno::CanNotOpenCamera, "Insta: camera open failed");
        }
    }

    discovery.FreeDeviceDescriptors(list);

    camera_->SetTimeout(std::max<int32_t>(1000, local_parameters_.get_CameraTimeoutMs()));

    const bool stream_mode = local_parameters_.get_WorkMode()._to_integral() == static_cast<int32_t>(WorkMode::Stream);
    is_stream_mode_.store(stream_mode);
    runtime_auto_download_.store(local_parameters_.get_AutoDownload());
    runtime_download_dir_ = local_parameters_.get_DownloadDir();

    if (local_parameters_.get_SyncLocalTime()) {
        std::time_t now = std::time(nullptr);
        if (!sync_local_time_to_camera(camera_, static_cast<uint64_t>(now))) {
            log::warn << "Insta: SyncLocalTimeToCamera failed" << log::endl;
        }
    }

    if (stream_mode) {
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

        // Note: Do NOT call SetVideoCaptureParams for FUNCTION_MODE_LIVE_STREAM in 2.0.2 SDK
        // It causes USB timeouts and device disconnect. StartLiveStreaming handles the mode setup.

        try {
            if (!camera_->StartLiveStreaming(param)) {
                camera_->Close();
                camera_.reset();
                stream_delegate_.reset();
                runtime_.reset();
                return handle_error(HeraErrno::CanNotOpenCamera, "Insta: StartLiveStreaming failed");
            }
        } catch (const std::exception& e) {
            if (camera_) {
                camera_->Close();
            }
            camera_.reset();
            stream_delegate_.reset();
            runtime_.reset();
            return handle_error(HeraErrno::CanNotOpenCamera,
                                std::string("Insta: StartLiveStreaming exception: ") + e.what());
        } catch (...) {
            if (camera_) {
                camera_->Close();
            }
            camera_.reset();
            stream_delegate_.reset();
            runtime_.reset();
            return handle_error(HeraErrno::CanNotOpenCamera, "Insta: StartLiveStreaming unknown exception");
        }

        stream_started_.store(true);
    } else {
        if (local_parameters_.get_AutoStartRecording()) {
            try {
                if (!start_recording_normal_video(
                        camera_, local_parameters_.get_VideoResolution(), local_parameters_.get_VideoBitrate())) {
                    camera_->Close();
                    camera_.reset();
                    runtime_.reset();
                    return handle_error(HeraErrno::CanNotOpenCamera, "Insta: StartRecording failed");
                }
            } catch (const std::exception& e) {
                if (camera_) {
                    camera_->Close();
                }
                camera_.reset();
                runtime_.reset();
                return handle_error(HeraErrno::CanNotOpenCamera,
                                    std::string("Insta: StartRecording exception: ") + e.what());
            } catch (...) {
                if (camera_) {
                    camera_->Close();
                }
                camera_.reset();
                runtime_.reset();
                return handle_error(HeraErrno::CanNotOpenCamera, "Insta: StartRecording unknown exception");
            }
            record_started_.store(true);
            log::info << "Insta: RecordDownload mode started recording" << log::endl;
        } else {
            log::info << "Insta: RecordDownload mode, waiting external start" << log::endl;
        }
    }

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
        if (!is_stream_mode_.load() && record_started_.load()) {
            try {
                record_started_.store(false);
                if (!stop_recording_with_optional_download(
                        camera_, runtime_auto_download_.load(), runtime_download_dir_)) {
                    log::warn << "Insta: stop recording completed with errors" << log::endl;
                }
            } catch (const std::exception& e) {
                log::warn << "Insta: StopRecording exception: " << e.what() << log::endl;
            } catch (...) {
                log::warn << "Insta: StopRecording unknown exception" << log::endl;
            }
        }

        if (is_stream_mode_.load() && stream_started_.load()) {
            // Some CameraSDK builds may crash in StopLiveStreaming() during teardown.
            // Closing the camera session directly is stable and sufficient for cleanup.
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

    if (!is_stream_mode_.load()) {
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
    if (type == "DownloadDir") {
        std::lock_guard<std::mutex> lock(camera_op_mutex_);
        if (!value.empty()) {
            runtime_download_dir_ = value;
            log::info << "Insta: runtime DownloadDir set to " << runtime_download_dir_ << log::endl;
        }
        return HeraErrno::Success;
    }

    if (type == "AutoDownload") {
        runtime_auto_download_.store(parse_bool(value, runtime_auto_download_.load()));
        log::info << "Insta: runtime AutoDownload set to " << (runtime_auto_download_.load() ? "true" : "false")
                  << log::endl;
        return HeraErrno::Success;
    }

    if (is_stream_mode_.load()) {
        return HeraErrno::Success;
    }

    if (!camera_) {
        return HeraErrno::CanNotOpenCamera;
    }

    if (type == "StartRecording") {
        std::lock_guard<std::mutex> lock(camera_op_mutex_);
        if (record_started_.load()) {
            return HeraErrno::Success;
        }
        try {
            if (!start_recording_normal_video(camera_, local_parameters_.get_VideoResolution(),
                                              local_parameters_.get_VideoBitrate())) {
                return handle_error(HeraErrno::CanNotOpenCamera, "Insta: StartRecording command failed");
            }
            record_started_.store(true);
            log::info << "Insta: StartRecording command success" << log::endl;
            return HeraErrno::Success;
        } catch (const std::exception& e) {
            return handle_error(HeraErrno::CanNotOpenCamera,
                                std::string("Insta: StartRecording command exception: ") + e.what());
        } catch (...) {
            return handle_error(HeraErrno::CanNotOpenCamera, "Insta: StartRecording command unknown exception");
        }
    }

    if (type == "StopRecording" || type == "StopAndDownload") {
        std::lock_guard<std::mutex> lock(camera_op_mutex_);
        if (!record_started_.load()) {
            return HeraErrno::Success;
        }
        try {
            const bool auto_download = (type == "StopAndDownload") ? true : runtime_auto_download_.load();
            if (!stop_recording_with_optional_download(camera_, auto_download, runtime_download_dir_)) {
                return handle_error(HeraErrno::CanNotOpenCamera, "Insta: StopRecording command failed");
            }
            record_started_.store(false);
            log::info << "Insta: " << type << " command success" << log::endl;
            return HeraErrno::Success;
        } catch (const std::exception& e) {
            return handle_error(HeraErrno::CanNotOpenCamera,
                                std::string("Insta: StopRecording command exception: ") + e.what());
        } catch (...) {
            return handle_error(HeraErrno::CanNotOpenCamera, "Insta: StopRecording command unknown exception");
        }
    }

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
