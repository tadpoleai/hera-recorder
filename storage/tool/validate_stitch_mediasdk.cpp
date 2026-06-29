///
/// @file validate_stitch_mediasdk.cpp
/// @brief Validate dual-fisheye → equirectangular stitching using MediaSDK RealTimeStitcher.
///
/// Reads a Stream-mode .hera file (produced by the insta plugin with
/// EnableInCameraStitching=OFF), feeds H.264 video and gyro packets to
/// ins::RealTimeStitcher, and saves the resulting equirectangular JPEG frames
/// to an output directory for visual inspection.
///
/// Compare the output with the ffmpeg v360 approach in ingest_stream_hera to
/// evaluate stitching quality.
///
/// Usage:
///   validate-stitch-mediasdk
///       --input   <stream_mode.hera>
///       --output  <output_dir>
///     [ --camera-name <name>     ]  e.g. "Insta360 X4", default: "Insta360 X4"
///     [ --decode  h264|h265      ]  default: h264
///     [ --stitch  template|optflow|dynamicstitch ]  default: template
///     [ --width   <px>           ]  output width,  default: 3840
///     [ --height  <px>           ]  output height, default: 1920
///     [ --max-frames <N>         ]  stop after N stitched frames, default: 30
///     [ --flowstate              ]  enable flowstate stabilisation
///

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <iomanip>

#include <sys/stat.h>   // mkdir
#include <ins_realtime_stitcher.h>
#ifdef HAVE_TURBOJPEG
#include <turbojpeg.h>
#endif

#ifdef HERA_COMPILE_IN_REPO
#include "storage/include/storage.hpp"
#include "device/include/device_data.hpp"
#else
#include <hera/storage/storage.hpp>
#include <hera/device/device_data.hpp>
#endif

using namespace wayz::hera;

// ── Packet layout (must match plugin_entry.cpp and extract_insta.cpp) ─────────

constexpr device::DeviceDataType kInstaVideoType = 0x0421;
constexpr device::DeviceDataType kInstaGyroType  = 0x0422;
constexpr size_t kDeviceDataHeaderBytes           = 24;

#pragma pack(push, 1)
struct VideoPacketHeader {
    int64_t  timestamp_device_ns;
    uint64_t timestamp_host_ns;
    uint8_t  stream_type;
    int32_t  stream_index;
    uint32_t payload_size;
};

struct GyroPacketHeader {
    uint64_t timestamp_host_ns;
    uint32_t sample_count;
    uint32_t payload_size;
};

// Identical to ins::GyroData — cast directly.
struct GyroSample {
    int64_t timestamp;
    double  ax, ay, az;
    double  gx, gy, gz;
};
#pragma pack(pop)

// ── Image save: JPEG via turbojpeg, or PPM fallback ──────────────────────────

static std::string image_extension()
{
#ifdef HAVE_TURBOJPEG
    return ".jpg";
#else
    return ".ppm";
#endif
}

static bool save_rgba_frame(const uint8_t* rgba, int width, int height,
                             const std::string& path, int quality = 85)
{
#ifdef HAVE_TURBOJPEG
    tjhandle tj = tjInitCompress();
    if (!tj) { std::cerr << "[stitch] tjInitCompress failed\n"; return false; }

    unsigned char* buf  = nullptr;
    unsigned long  size = 0;
    int rc = tjCompress2(tj, rgba, width, 0, height, TJPF_RGBA,
                         &buf, &size, TJSAMP_420, quality, TJFLAG_FASTDCT);
    if (rc != 0) {
        std::cerr << "[stitch] tjCompress2: " << tjGetErrorStr(tj) << "\n";
        tjDestroy(tj); return false;
    }
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) { tjFree(buf); tjDestroy(tj); return false; }
    fwrite(buf, 1, size, f);
    fclose(f);
    tjFree(buf);
    tjDestroy(tj);
    return true;
#else
    // PPM: binary RGB (no alpha)
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) { std::cerr << "[stitch] cannot write: " << path << "\n"; return false; }
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    for (int i = 0; i < width * height; ++i) {
        fwrite(rgba + i * 4, 1, 3, f);  // RGBA → RGB
    }
    fclose(f);
    return true;
#endif
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    // ── CLI arguments ─────────────────────────────────────────────────────────
    std::string input_hera;
    std::string output_dir  = "./stitch_output";
    std::string camera_name = "Insta360 X4";
    std::string decode_str  = "h264";
    std::string stitch_str  = "template";
    int out_w        = 3840;
    int out_h        = 1920;
    int max_frames   = 30;
    bool flowstate   = false;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if      ((a == "--input"   || a == "-i") && i+1 < argc) input_hera  = argv[++i];
        else if ((a == "--output"  || a == "-o") && i+1 < argc) output_dir  = argv[++i];
        else if  (a == "--camera-name"           && i+1 < argc) camera_name = argv[++i];
        else if  (a == "--decode"                && i+1 < argc) decode_str  = argv[++i];
        else if  (a == "--stitch"                && i+1 < argc) stitch_str  = argv[++i];
        else if  (a == "--width"                 && i+1 < argc) out_w       = std::stoi(argv[++i]);
        else if  (a == "--height"                && i+1 < argc) out_h       = std::stoi(argv[++i]);
        else if  (a == "--max-frames"            && i+1 < argc) max_frames  = std::stoi(argv[++i]);
        else if  (a == "--flowstate")                            flowstate   = true;
        else if  (a == "--help" || a == "-h") {
            std::cout <<
                "Usage: validate-stitch-mediasdk\n"
                "  --input        <stream_mode.hera>\n"
                "  --output       <output_dir>          default: ./stitch_output\n"
                "  --camera-name  <name>                default: \"Insta360 X4\"\n"
                "  --decode       h264|h265             default: h264\n"
                "  --stitch       template|optflow|dynamicstitch  default: template\n"
                "  --width        <px>                  default: 3840\n"
                "  --height       <px>                  default: 1920\n"
                "  --max-frames   <N>                   default: 30\n"
                "  --flowstate                          enable flowstate\n";
            return 0;
        }
    }

    if (input_hera.empty()) {
        std::cerr << "Error: --input is required\n";
        return 1;
    }

    // ── Resolve parameters ────────────────────────────────────────────────────
    ins::VideoDecodeType decode_type =
        (decode_str == "h265") ? ins::VideoDecodeType::kH265
                               : ins::VideoDecodeType::kH264;

    ins::STITCH_TYPE stitch_type = ins::STITCH_TYPE::TEMPLATE;
    if      (stitch_str == "optflow")       stitch_type = ins::STITCH_TYPE::OPTFLOW;
    else if (stitch_str == "dynamicstitch") stitch_type = ins::STITCH_TYPE::DYNAMICSTITCH;

    mkdir(output_dir.c_str(), 0755);

    // ── Stitcher setup ────────────────────────────────────────────────────────
    ins::InitEnv();
    ins::SetLogLevel(ins::InsLogLevel::WARNING);

    // CameraInfo: cameraName and decode_type are the critical fields.
    // offset and window_crop_info_ are left at zero — the SDK uses its internal
    // calibration table for known camera models when offset is empty.
    ins::CameraInfo cam_info;
    cam_info.cameraName  = camera_name;
    cam_info.decode_type = decode_type;
    // offset left empty → SDK uses default calibration for this camera

    auto stitcher = std::make_shared<ins::RealTimeStitcher>();
    stitcher->SetCameraInfo(cam_info);
    stitcher->SetStitchType(stitch_type);
    stitcher->SetOutputSize(out_w, out_h);
    stitcher->EnableFlowState(flowstate);
    stitcher->SetSoftwareCodecUsage(false, true);  // software decode, HW encode (unused here)

    // ── Frame collection state ────────────────────────────────────────────────
    std::mutex              frame_mtx;
    std::condition_variable frame_cv;
    std::atomic<int>        saved_count{0};

    stitcher->SetStitchRealTimeDataCallback(
        [&](uint8_t* data[4], int /*linesize*/[4],
            int width, int height, int /*format*/, int64_t timestamp)
        {
            const int idx = saved_count.load();
            if (idx >= max_frames) return;

            std::ostringstream path;
            path << output_dir << "/frame_"
                 << std::setfill('0') << std::setw(4) << idx
                 << image_extension();

            if (save_rgba_frame(data[0], width, height, path.str())) {
                std::cout << "[stitch] frame " << idx
                          << " ts=" << timestamp
                          << " -> " << path.str() << "\n";
            }

            if (++saved_count >= max_frames) {
                frame_cv.notify_all();
            }
        });

    stitcher->SetStitchStateCallback(
        [&](int error, const char* info)
        {
            std::cerr << "[stitch] error " << error << ": " << info << "\n";
            frame_cv.notify_all();
        });

    std::cout << "[stitch] camera: " << camera_name
              << "  decode: " << decode_str
              << "  stitch: " << stitch_str
              << "  output: " << out_w << "x" << out_h
              << "  max_frames: " << max_frames << "\n";

    stitcher->StartStitch();

    // ── Feed hera packets ─────────────────────────────────────────────────────
    auto storage = storage::StorageManager::open(input_hera, true);
    if (!storage) {
        std::cerr << "[stitch] cannot open: " << input_hera << "\n";
        return 1;
    }

    int64_t video_fed = 0, gyro_fed = 0;
    auto t0 = std::chrono::steady_clock::now();

    while (auto pkt = storage->read()) {
        if (!pkt) continue;

        // Stop feeding once we have enough frames
        if (saved_count.load() >= max_frames) break;

        const uint8_t* raw     = reinterpret_cast<const uint8_t*>(pkt.get());
        const uint8_t* payload = raw + kDeviceDataHeaderBytes;
        const size_t   plen    = pkt->get_length() - kDeviceDataHeaderBytes;

        if (pkt->is_type(kInstaVideoType)) {
            if (plen < sizeof(VideoPacketHeader)) continue;
            const auto* vh = reinterpret_cast<const VideoPacketHeader*>(payload);
            const uint8_t* vdata = payload + sizeof(VideoPacketHeader);
            const uint32_t vsize = vh->payload_size;
            if (vsize == 0 || vsize > plen - sizeof(VideoPacketHeader)) continue;

            stitcher->HandleVideoData(vdata, vsize,
                                      vh->timestamp_device_ns,
                                      vh->stream_type,
                                      vh->stream_index);
            ++video_fed;

        } else if (pkt->is_type(kInstaGyroType)) {
            if (plen < sizeof(GyroPacketHeader)) continue;
            const auto* gh = reinterpret_cast<const GyroPacketHeader*>(payload);
            const uint32_t max_samples =
                gh->payload_size / static_cast<uint32_t>(sizeof(GyroSample));
            const uint32_t n = std::min(gh->sample_count, max_samples);
            if (n == 0) continue;

            // GyroSample is binary-identical to ins::GyroData — cast directly.
            const auto* samples =
                reinterpret_cast<const ins::GyroData*>(payload + sizeof(GyroPacketHeader));
            std::vector<ins::GyroData> gyro_vec(samples, samples + n);
            stitcher->HandleGyroData(gyro_vec);
            ++gyro_fed;
        }
    }

    std::cout << "[stitch] fed " << video_fed << " video packets, "
              << gyro_fed << " gyro packets\n";

    // Wait for stitcher to flush remaining frames (up to 5 s)
    {
        std::unique_lock<std::mutex> lk(frame_mtx);
        frame_cv.wait_for(lk, std::chrono::seconds(5),
                          [&]{ return saved_count.load() >= max_frames; });
    }

    stitcher->CancelStitch();

    auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();

    std::cout << "[stitch] done: " << saved_count.load() << " frames saved"
              << " in " << std::fixed << std::setprecision(1) << elapsed << "s"
              << " -> " << output_dir << "/\n";

    if (saved_count.load() == 0) {
        std::cerr << "[stitch] WARNING: no frames produced.\n"
                  << "  Check --camera-name matches the recording device.\n"
                  << "  Try --decode h265 if camera recorded in H.265 mode.\n"
                  << "  Verify the .hera file contains stream_index=0 video packets.\n";
        return 1;
    }

    return 0;
}
