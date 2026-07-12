///
/// @file plugin_entry.cpp
/// @brief Insta360 phase-1 plugin: realtime preview bitstream + gyro raw capture
///

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <cstdlib>
#include <cctype>
#include <iomanip>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <unistd.h>

#include "plugin_common.hpp"
#include "plugin_param.hpp"
#include "data/camera_data.hpp"

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

// RecordDownload sessions are 8K/5.7K per-lens; refuse to start a new recording
// with less than this much free space on the SD card rather than let it fail
// mid-session.
constexpr uint64_t kMinFreeStorageBytes = 2ull * 1024 * 1024 * 1024;  // 2 GiB

bool has_enough_storage(const std::shared_ptr<ins_camera::Camera>& camera, const uint64_t min_free_bytes)
{
    if (!camera) {
        return false;
    }
    ins_camera::StorageStatus status{};
    if (!camera->GetStorageState(status)) {
        log::warn << "Insta: GetStorageState failed, proceeding without a free-space check" << log::endl;
        return true;
    }
    if (status.state == ins_camera::CardState::STOR_CS_NOCARD) {
        log::warn << "Insta: no SD card detected" << log::endl;
        return false;
    }
    if (status.state == ins_camera::CardState::STOR_CS_WPCARD) {
        log::warn << "Insta: SD card is write-protected" << log::endl;
        return false;
    }
    if (status.state != ins_camera::CardState::STOR_CS_PASS && status.state != ins_camera::CardState::STOR_CS_NOSPACE) {
        log::warn << "Insta: SD card in bad state (" << static_cast<int>(status.state) << ")" << log::endl;
        return false;
    }
    if (status.free_space < min_free_bytes) {
        log::warn << "Insta: SD card free space too low: " << status.free_space << " bytes (need "
                  << min_free_bytes << ")" << log::endl;
        return false;
    }
    return true;
}

std::string basename_from_url(const std::string& url)
{
    const auto slash = url.find_last_of("/\\");
    if (slash == std::string::npos || slash + 1 >= url.size()) {
        return "insta_record.mp4";
    }
    return url.substr(slash + 1);
}

// Strips directory and ".hera" extension from a StorageManager::filename()-style path,
// e.g. "/var/hera/data/20260711180536_fred_office.hera" -> "20260711180536_fred_office".
// Returns "" if hera_path is empty (e.g. no storage attached), signaling callers to fall
// back to their pre-existing naming instead of renaming to an empty string.
std::string hera_basename(const std::string& hera_path)
{
    if (hera_path.empty()) {
        return "";
    }
    const auto slash = hera_path.find_last_of("/\\");
    std::string base = (slash == std::string::npos) ? hera_path : hera_path.substr(slash + 1);
    static const std::string kSuffix = ".hera";
    if (base.size() > kSuffix.size() && base.compare(base.size() - kSuffix.size(), kSuffix.size(), kSuffix) == 0) {
        base = base.substr(0, base.size() - kSuffix.size());
    }
    return base;
}

// Returns list of successfully downloaded local paths (empty on failure).
// `hera_session_path` (StorageManager::filename()) drives renaming the downloaded file(s)
// to share the .hera session's basename, so .hera/.insv/.session.json can be correlated
// and copied around as a group by filename alone instead of relying on a single shared
// insta_session.json that gets overwritten every session.
std::vector<std::string> download_media_urls(const std::shared_ptr<ins_camera::Camera>& camera,
                                             const ins_camera::MediaUrl& media_url,
                                             const std::string& output_dir,
                                             const std::string& hera_session_path)
{
    if (!camera || media_url.Empty()) {
        return {};
    }

    if (access(output_dir.c_str(), F_OK) != 0) {
        log::warn << "Insta: download directory does not exist: " << output_dir << log::endl;
        return {};
    }

    static constexpr int kDownloadRetries = 3;
    static constexpr auto kDownloadRetryDelay = std::chrono::milliseconds(1000);

    const std::string session_basename = hera_basename(hera_session_path);

    std::vector<std::string> downloaded;
    const auto& urls = media_url.OriginUrls();
    for (size_t i = 0; i < urls.size(); ++i) {
        const auto& remote = urls[i];
        const auto local = join_path(output_dir, basename_from_url(remote));

        bool ok = false;
        for (int attempt = 1; attempt <= kDownloadRetries; ++attempt) {
            ok = camera->DownloadCameraFile(remote, local);
            if (ok) {
                break;
            }
            log::warn << "Insta: download attempt " << attempt << "/" << kDownloadRetries
                      << " failed: " << remote << " -> " << local << log::endl;
            if (attempt < kDownloadRetries) {
                std::this_thread::sleep_for(kDownloadRetryDelay);
            }
        }
        if (!ok) {
            log::warn << "Insta: giving up on " << remote << " after " << kDownloadRetries
                      << " attempts; leaving it on the SD card" << log::endl;
            continue;
        }

        // The SDK may save the file with a different extension than the URL suggests
        // (e.g. URL says .insv but file lands as .mp4 when stitching mode changes format).
        // Find the actual file on disk.
        std::string actual = local;
        if (access(local.c_str(), F_OK) != 0) {
            static const std::vector<std::string> kAltExts = {".mp4", ".insv", ".MP4", ".INSV"};
            const auto dot = local.rfind('.');
            const std::string stem = (dot != std::string::npos) ? local.substr(0, dot) : local;
            for (const auto& ext : kAltExts) {
                const std::string candidate = stem + ext;
                if (access(candidate.c_str(), F_OK) == 0) {
                    actual = candidate;
                    log::info << "Insta: SDK saved file as " << actual
                              << " (URL had " << local << ")" << log::endl;
                    break;
                }
            }
        }

        log::info << "Insta: downloaded " << actual << log::endl;

        // Rename to share the .hera session's basename (see comment on this function).
        // Falls back to leaving the SDK's own filename alone if there's no session basename
        // (e.g. no storage attached) or the rename fails for some reason.
        std::string final_path = actual;
        if (!session_basename.empty()) {
            const auto dot = actual.rfind('.');
            const std::string ext = (dot != std::string::npos) ? actual.substr(dot) : ".insv";
            std::ostringstream target_name;
            target_name << session_basename;
            if (urls.size() > 1) {
                target_name << "_" << std::setfill('0') << std::setw(2) << i;
            }
            target_name << ext;
            const std::string target_path = join_path(output_dir, target_name.str());
            if (target_path != actual) {
                if (std::rename(actual.c_str(), target_path.c_str()) == 0) {
                    final_path = target_path;
                    log::info << "Insta: renamed to " << final_path << log::endl;
                } else {
                    log::warn << "Insta: failed to rename " << actual << " -> " << target_path
                              << " (" << std::strerror(errno) << "), keeping original name" << log::endl;
                }
            }
        }
        downloaded.push_back(final_path);

        // NOTE: intentionally NOT calling camera->DeleteCameraFile(remote) here yet —
        // auto-deleting footage from the SD card needs an explicit go-ahead before it
        // ships. See TODO in the retention-policy discussion; SD card accumulation must
        // be handled some other way (manual cleanup / separate opt-in tool) until then.
    }

    return downloaded;
}

std::string json_escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}

// Write a minimal JSON sidecar so hera-storage-ingest-insta-video can locate
// the MP4 and anchor frames to the hera timeline via record_start_host_ns.
// `hera_session_path` is this run's .hera file (StorageManager::filename()) — an explicit
// correlation key, so ingest doesn't have to guess which .hera a .insv belongs to purely
// from directory/timestamp proximity.
void write_session_json(const std::string& output_dir,
                        uint64_t record_start_host_ns,
                        const std::vector<std::string>& mp4_paths,
                        const std::string& hera_session_path)
{
    const std::string base = hera_basename(hera_session_path);
    const std::string filename = base.empty() ? "insta_session.json" : (base + ".session.json");
    const auto path = join_path(output_dir, filename);
    std::ofstream f(path);
    if (!f) {
        log::warn << "Insta: cannot write session JSON to " << path << log::endl;
        return;
    }

    f << "{\n";
    f << "  \"record_start_host_ns\": " << record_start_host_ns << ",\n";
    f << "  \"device_vendor_type\": 1057,\n";  // 0x0421
    f << "  \"hera_session_path\": \"" << json_escape(hera_session_path) << "\",\n";
    f << "  \"mp4_files\": [";
    for (size_t i = 0; i < mp4_paths.size(); ++i) {
        if (i > 0) f << ", ";
        f << "\"" << json_escape(mp4_paths[i]) << "\"";
    }
    f << "]\n}\n";

    log::info << "Insta: session metadata -> " << path << log::endl;
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

// Returns downloaded local MP4 paths (empty if download skipped or failed).
std::vector<std::string> stop_recording_with_optional_download(
    const std::shared_ptr<ins_camera::Camera>& camera,
    const bool auto_download,
    const std::string& download_dir,
    const std::string& hera_session_path)
{
    if (!camera) {
        return {};
    }

    const auto media_url = camera->StopRecording();
    if (media_url.Empty()) {
        log::warn << "Insta: StopRecording returned empty media url" << log::endl;
        return {};
    }

    if (!auto_download) {
        for (const auto& url : media_url.OriginUrls()) {
            log::info << "Insta: recorded media url " << url << log::endl;
        }
        return {};
    }

    return download_media_urls(camera, media_url, download_dir, hera_session_path);
}

// Resolutions for USB live-streaming (equirectangular, 2:1 ratio).
ins_camera::VideoResolution map_video_resolution(const StreamResolution value)
{
    if (value._to_integral() == static_cast<int32_t>(StreamResolution::R2560x1280P30)) {
        return ins_camera::VideoResolution::RES_2560_1280P30;
    }
    if (value._to_integral() == static_cast<int32_t>(StreamResolution::R1440x720P30)) {
        return ins_camera::VideoResolution::RES_1440_720P30;
    }
    return ins_camera::VideoResolution::RES_3840_1920P30;
}

// Resolutions for SD card recording (square per-lens, as used in SDK demo option 6).
// EnableInCameraStitching() then stitches the two lenses to equirectangular on-device.
// Unlike the old VideoResolution-based mapping this replaced, each RecordResolution
// option maps to exactly one distinct SDK resolution -- no more "R1440x720P30" secretly
// producing a full 5.7K recording.
ins_camera::VideoResolution map_record_resolution(const RecordResolution value)
{
    if (value._to_integral() == static_cast<int32_t>(RecordResolution::R8K)) {
        return ins_camera::VideoResolution::RES_3840_3840P30;  // 8K per-lens
    }
    return ins_camera::VideoResolution::RES_2880_2880P30;  // R57K (default): 5.7K per-lens
}

ins_camera::VideoResolution map_lrv_resolution(const bool fixed_lrv_resolution)
{
    (void)fixed_lrv_resolution;
    return ins_camera::VideoResolution::RES_1440_720P30;
}

bool start_recording_normal_video(const std::shared_ptr<ins_camera::Camera>& camera,
                                  const RecordResolution resolution,
                                  const int32_t video_bitrate,
                                  const bool enable_stitching = false)
{
    if (!camera) {
        return false;
    }

    if (!has_enough_storage(camera, kMinFreeStorageBytes)) {
        log::warn << "Insta: refusing to start recording, insufficient SD card space" << log::endl;
        return false;
    }

    // Mirror the SDK demo's two-step flow (option 12 then option 6):
    //   1. EnableInCameraStitching  — called standalone BEFORE any mode change
    //   2. SetVideoSubMode          — may reset camera mode state
    //   3. SetVideoCaptureParams    — sets per-lens square resolution for SD recording
    //   4. StartRecording
    //
    // SetVideoSubMode after EnableInCameraStitching (not before) avoids the firmware
    // clearing the stitching flag during a mode transition.
    if (enable_stitching) {
        if (!camera->EnableInCameraStitching(true)) {
            log::warn << "Insta: EnableInCameraStitching failed — recording will be dual-fisheye" << log::endl;
        } else {
            log::info << "Insta: EnableInCameraStitching OK" << log::endl;
        }
    }

    if (!camera->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_NORMAL)) {
        log::warn << "Insta: SetVideoSubMode(VIDEO_NORMAL) failed" << log::endl;
    }

    // SD card recording uses square per-lens resolution (e.g. RES_3840_3840P30),
    // matching demo option 6. The 2:1 equirectangular resolutions are for USB streaming only.
    ins_camera::RecordParams record_params;
    record_params.resolution = map_record_resolution(resolution);
    record_params.bitrate = std::max<int32_t>(131072, video_bitrate);

    if (!camera->SetVideoCaptureParams(record_params, ins_camera::CameraFunctionMode::FUNCTION_MODE_NORMAL_VIDEO)) {
        log::warn << "Insta: SetVideoCaptureParams(FUNCTION_MODE_NORMAL_VIDEO) failed" << log::endl;
    }

    return camera->StartRecording();
}

// SDK 2.1.1 changed SyncLocalTimeToCamera(uint64_t utc, uint32_t tz_offset_s).
// Compute the local timezone offset so the camera shows local time.
template <typename CameraType>
auto sync_local_time_impl(CameraType* cam, uint64_t t, int) -> decltype(cam->SyncLocalTimeToCamera(t, 0u), bool())
{
    std::time_t t_val = static_cast<std::time_t>(t);
    std::tm tm{};
    localtime_r(&t_val, &tm);
    const auto tz_offset = static_cast<uint32_t>(timegm(&tm) - t_val);
    return cam->SyncLocalTimeToCamera(t, tz_offset);
}

// SDK 2.0.x single-argument fallback.
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
    HeraStreamDelegate(const std::shared_ptr<RuntimeState>& runtime, bool enable_gyro, bool capture_video = true) :
        runtime_(runtime), enable_gyro_(enable_gyro), capture_video_(capture_video)
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
        if (!runtime_ || !capture_video_ || !data || size == 0) {
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
    bool capture_video_;
};

std::shared_ptr<RuntimeState> runtime_;
std::shared_ptr<ins_camera::Camera> camera_;
std::shared_ptr<ins_camera::StreamDelegate> stream_delegate_;
std::atomic<bool> stream_started_{false};
std::atomic<bool> is_stream_mode_{true};
std::atomic<bool> record_started_{false};
// Separate from stream_started_: that flag means "Stream-mode's own live view is
// active"; this one means "a gyro-only live stream is running alongside a
// RecordDownload SD-card recording" -- distinct modes, distinct lifecycles.
std::atomic<bool> gyro_stream_started_{false};
std::atomic<bool> runtime_auto_download_{true};
std::string runtime_download_dir_{"."};
std::mutex camera_op_mutex_;
uint64_t record_start_host_ns_{0};
// Device::stop() forces is_record_ (get_record()) to false *before* calling
// disconnect(), so disconnect() can never observe "was the daemon-level 录制数据
// switch on" by reading get_record() itself. fetch() runs continuously on the
// same session and can still see the true value while it lasts, so it latches
// it here; disconnect() reads the latch instead. Reset at the top of connect()
// so each session starts fresh.
std::atomic<bool> recording_ever_enabled_{false};

// Starts a gyro-only live stream (no video/audio) alongside an already-running
// SD-card recording, so RecordDownload sessions still get continuous
// InstaGyroPacket data into .hera (needed for lidar-camera time sync). Must be
// called AFTER StartRecording() -- verified on real hardware that calling
// SetVideoSubMode/SetVideoCaptureParams here (like Stream mode does before its
// own StartLiveStreaming) resets camera mode state and would disrupt the
// already-active recording, so those calls are deliberately skipped.
// Failure here is non-fatal: video recording already succeeded, so losing
// gyro is an acceptable degradation, not a reason to fail connect().
bool start_gyro_only_stream(const std::shared_ptr<ins_camera::Camera>& camera,
                            std::shared_ptr<ins_camera::StreamDelegate>& out_delegate,
                            const std::shared_ptr<RuntimeState>& runtime)
{
    if (!camera) {
        return false;
    }

    out_delegate = std::make_shared<HeraStreamDelegate>(runtime, /*enable_gyro=*/true, /*capture_video=*/false);
    camera->SetStreamDelegate(out_delegate);

    ins_camera::LiveStreamParam param;
    param.enable_video = false;
    param.enable_audio = false;
    param.enable_gyro = true;
    param.using_lrv = false;
    // The SDK does not fully honor enable_video=false (video packets still
    // arrive), so pick the smallest resolution to at least minimize the
    // wasted bandwidth/CPU from the video pipe we're going to discard.
    param.video_resolution = ins_camera::VideoResolution::RES_1440_720P30;

    try {
        if (!camera->StartLiveStreaming(param)) {
            log::warn << "Insta: gyro-only StartLiveStreaming failed -- RecordDownload continues without gyro"
                      << log::endl;
            return false;
        }
    } catch (const std::exception& e) {
        log::warn << "Insta: gyro-only StartLiveStreaming exception: " << e.what()
                  << " -- RecordDownload continues without gyro" << log::endl;
        return false;
    } catch (...) {
        log::warn << "Insta: gyro-only StartLiveStreaming unknown exception -- RecordDownload continues without gyro"
                  << log::endl;
        return false;
    }
    return true;
}

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
    recording_ever_enabled_.store(false);
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

        // SDK 2.1.1: EnableInCameraStitching must be called BEFORE SetVideoSubMode/
        // SetVideoCaptureParams — once the mode is committed the stitching flag is locked.
        // With 2.1.1 this actually works for the live stream: OnVideoData receives
        // stitched equirectangular frames directly when enabled.
        if (local_parameters_.get_EnableInCameraStitching()) {
            if (!camera_->EnableInCameraStitching(true)) {
                log::warn << "Insta: EnableInCameraStitching (stream) failed" << log::endl;
            } else {
                log::info << "Insta: EnableInCameraStitching OK — stream will be equirectangular" << log::endl;
            }
        }

        ins_camera::LiveStreamParam param;
        param.video_resolution = map_video_resolution(local_parameters_.get_StreamResolution());
        param.lrv_video_resulution = map_lrv_resolution(local_parameters_.get_FixedLrvResolution());
        param.video_bitrate = std::max<int32_t>(131072, local_parameters_.get_VideoBitrate());
        param.enable_audio = local_parameters_.get_EnableAudio();
        param.using_lrv = local_parameters_.get_UsingLrv();

        // SDK 2.1.1 requires SetVideoSubMode(VIDEO_LIVEVIEW) + SetVideoCaptureParams
        // before StartLiveStreaming for X4/X5. (SDK 2.0.x did not have VIDEO_LIVEVIEW
        // and calling SetVideoCaptureParams before StartLiveStreaming caused USB timeouts.)
        if (!camera_->SetVideoSubMode(ins_camera::SubVideoMode::VIDEO_LIVEVIEW)) {
            log::warn << "Insta: SetVideoSubMode(VIDEO_LIVEVIEW) failed" << log::endl;
        }
        {
            ins_camera::RecordParams rp;
            rp.resolution = param.video_resolution;
            rp.bitrate = 0;
            if (!camera_->SetVideoCaptureParams(rp, ins_camera::CameraFunctionMode::FUNCTION_MODE_LIVE_STREAM)) {
                log::warn << "Insta: SetVideoCaptureParams(FUNCTION_MODE_LIVE_STREAM) failed" << log::endl;
            }
        }

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
                const bool stitching = local_parameters_.get_EnableInCameraStitching();
                if (!start_recording_normal_video(
                        camera_, local_parameters_.get_RecordResolution(), local_parameters_.get_VideoBitrate(),
                        stitching)) {
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
            record_start_host_ns_ = static_cast<uint64_t>(time::Timestamp::now());
            log::info << "Insta: RecordDownload mode started recording (start_ns=" << record_start_host_ns_ << ")" << log::endl;

            if (local_parameters_.get_EnableGyro()) {
                gyro_stream_started_.store(start_gyro_only_stream(camera_, stream_delegate_, runtime_));
                log::info << "Insta: gyro-only stream " << (gyro_stream_started_.load() ? "started" : "NOT started")
                          << log::endl;
            }
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
                // Only pull the SD-card video onto disk if 录制数据 (setRecord) was ever
                // actually turned on this session. The camera keeps recording internally
                // regardless (AutoStartRecording is independent of the daemon's recording_
                // flag), but downloading footage the operator never asked to persist just
                // pollutes the download dir. The recording stays on the SD card either way
                // (no auto-delete), so nothing is lost -- it's just not copied off.
                const bool should_download = runtime_auto_download_.load() && recording_ever_enabled_.load();
                const auto downloaded = stop_recording_with_optional_download(
                    camera_, should_download, runtime_download_dir_, storage_filename());
                if (!recording_ever_enabled_.load()) {
                    log::info << "Insta: 录制数据 was never enabled this session, skipping download"
                              << log::endl;
                }
                if (downloaded.empty() && should_download) {
                    log::warn << "Insta: stop recording completed with errors" << log::endl;
                }
                if (!downloaded.empty() && record_start_host_ns_ != 0) {
                    write_session_json(runtime_download_dir_, record_start_host_ns_, downloaded,
                                       storage_filename());
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
        if (gyro_stream_started_.load()) {
            // Same StopLiveStreaming() crash risk as above applies to the RecordDownload
            // gyro-only stream -- skip it and let Close() below tear both sessions down
            // together. NOTE: StopRecording() above therefore runs while the gyro stream
            // is still active -- the real-hardware test that validated concurrent
            // record+stream stopped the stream *before* StopRecording, so this exact
            // ordering (stop recording first, stream still running) has not itself been
            // verified yet. Confirm on real hardware that the downloaded file is intact.
            gyro_stream_started_.store(false);
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

    if (get_record()) {
        recording_ever_enabled_.store(true);
    }

    // No is_stream_mode_ gate here: RecordDownload sessions with a gyro-only
    // stream running also need to drain this queue. When no stream_delegate_
    // is active (RecordDownload with gyro disabled), the queue is simply
    // always empty and cv.wait_for() below times out naturally.

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
            if (!start_recording_normal_video(camera_, local_parameters_.get_RecordResolution(),
                                              local_parameters_.get_VideoBitrate(),
                                              local_parameters_.get_EnableInCameraStitching())) {
                return handle_error(HeraErrno::CanNotOpenCamera, "Insta: StartRecording command failed");
            }
            record_started_.store(true);
            log::info << "Insta: StartRecording command success" << log::endl;

            if (local_parameters_.get_EnableGyro()) {
                gyro_stream_started_.store(start_gyro_only_stream(camera_, stream_delegate_, runtime_));
                log::info << "Insta: gyro-only stream " << (gyro_stream_started_.load() ? "started" : "NOT started")
                          << log::endl;
            }
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
            const auto downloaded = stop_recording_with_optional_download(
                camera_, auto_download, runtime_download_dir_, storage_filename());
            if (auto_download && downloaded.empty()) {
                return handle_error(HeraErrno::CanNotOpenCamera, "Insta: StopRecording command failed");
            }
            if (!downloaded.empty() && record_start_host_ns_ != 0) {
                write_session_json(runtime_download_dir_, record_start_host_ns_, downloaded,
                                   storage_filename());
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
    (void)parameters;
    if (storage_data->is_type(InstaJpegFramePacket::TypeVal)) {
        auto* raw = static_cast<InstaJpegFramePacket*>(storage_data.get());
        const uint32_t jpeg_size = raw->payload_size;
        const uint32_t length = static_cast<uint32_t>(sizeof(data::CompressedImage)) + jpeg_size;
        auto sensor_data = data::SensorData::create_from(
            storage_data, SensorDataType::CompressedImage, length);
        auto* img = static_cast<data::CompressedImage*>(sensor_data.get());
        img->timestamp_intrinsic_ns  = raw->timestamp_host_ns;
        img->compress_format         = data::CompressedImage::CompressFormat::JPEG;
        img->image_data_size         = jpeg_size;
        std::memcpy(img->image_data, raw->payload, jpeg_size);
        return sensor_data;
    }
    return data::SensorData::broken_data();
}

}  // namespace insta
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz
