///
/// @file ingest_insta_video.cpp
/// @brief Decode the MP4/INSV downloaded after a RecordDownload session and write
///        JPEG frames as InstaJpegFramePacket (0x0423) into a new .hera file.
///
/// When built with -DHAVE_MEDIASDK (amd64, libMediaSDK installed), uses
/// Insta360's VideoStitcher for lens-calibrated equirectangular output.
/// Otherwise falls back to ffmpeg with a v360=dfisheye:e filter.
///
/// Usage:
///   hera-storage-ingest-insta-video
///       --session   <insta_session.json>
///       --output    <camera.hera>
///     [ --fps       <rate>         ]  default: 1
///     [ --quality   <1-31>         ]  JPEG quality (ffmpeg -q:v), default 3
///     [ --width     <pixels>       ]  output width,  default 3840
///     [ --height    <pixels>       ]  output height, default 1920
///     [ --model-dir <path>         ]  MediaSDK AI models dir (optional)
///     [ --no-stitch               ]  skip stitching, write raw fisheye frames
///     [ --fov       <degrees>      ]  per-lens FOV for ffmpeg v360, default 190
///

#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#ifdef HAVE_MEDIASDK
#include <ins_stitcher.h>
#endif

#ifdef HERA_COMPILE_IN_REPO
#include "storage/include/storage.hpp"
#include "device/include/device_data.hpp"
#else
#include <hera/storage/storage.hpp>
#include <hera/device/device_data.hpp>
#endif

using namespace wayz::hera;

// ── Packet constants ──────────────────────────────────────────────────────────

constexpr device::DeviceVendorType kInstaVendorType = 0x0421;
constexpr device::DeviceDataType   kInstaJpegType   = 0x0423;

// DeviceData header: length(4) + device_id(4) + vendor_type(2) + msg_type(2) +
//                    sequence(4) + timestamp_receive_ns(8) = 24 bytes.
constexpr size_t kDeviceDataHeaderBytes = 24;

#pragma pack(push, 1)
struct InstaJpegFrameHeader {
    uint64_t timestamp_host_ns;
    uint32_t frame_index;
    uint32_t width;
    uint32_t height;
    uint32_t payload_size;
};
#pragma pack(pop)

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string run_command(const std::string& cmd)
{
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return result;
    char buf[256];
    while (fgets(buf, sizeof(buf), pipe)) {
        result += buf;
    }
    pclose(pipe);
    return result;
}

static std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static std::vector<uint8_t> read_file(const std::string& path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    auto size = f.tellg();
    if (size <= 0) return {};
    f.seekg(0);
    std::vector<uint8_t> buf(static_cast<size_t>(size));
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

// ── JSON helpers ──────────────────────────────────────────────────────────────

static uint64_t json_extract_u64(const std::string& json, const std::string& key)
{
    const std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return 0;
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return 0;
    while (pos < json.size() && (json[pos] == ':' || json[pos] == ' ')) ++pos;
    return std::stoull(json.substr(pos));
}

static std::vector<std::string> json_extract_string_array(const std::string& json,
                                                           const std::string& key)
{
    std::vector<std::string> result;
    const std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return result;
    pos = json.find('[', pos);
    if (pos == std::string::npos) return result;
    auto end = json.find(']', pos);
    if (end == std::string::npos) return result;
    std::string arr = json.substr(pos + 1, end - pos - 1);
    size_t i = 0;
    while (i < arr.size()) {
        auto q1 = arr.find('"', i);
        if (q1 == std::string::npos) break;
        auto q2 = arr.find('"', q1 + 1);
        if (q2 == std::string::npos) break;
        result.push_back(arr.substr(q1 + 1, q2 - q1 - 1));
        i = q2 + 1;
    }
    return result;
}

// ── Write one InstaJpegFramePacket to the hera output ─────────────────────────

static void write_jpeg_packet(
    const std::vector<uint8_t>& jpeg,
    uint64_t frame_ts_ns,
    uint32_t frame_index,
    uint32_t width, uint32_t height,
    uint32_t device_id,
    uint32_t& global_seq,
    storage::StorageManager& dst)
{
    const uint32_t total_len = static_cast<uint32_t>(
        kDeviceDataHeaderBytes + sizeof(InstaJpegFrameHeader) + jpeg.size());
    auto pkt = device::data::DeviceData::create(
        total_len, device_id, kInstaVendorType, kInstaJpegType, global_seq++);
    if (!pkt) { std::cerr << "[ingest] DeviceData::create failed\n"; return; }

    auto* hdr = reinterpret_cast<InstaJpegFrameHeader*>(
        reinterpret_cast<uint8_t*>(pkt.get()) + kDeviceDataHeaderBytes);
    hdr->timestamp_host_ns = frame_ts_ns;
    hdr->frame_index       = frame_index;
    hdr->width             = width;
    hdr->height            = height;
    hdr->payload_size      = static_cast<uint32_t>(jpeg.size());
    std::memcpy(hdr + 1, jpeg.data(), jpeg.size());
    dst.add_data(device_id, pkt, true);
}

// ── ffmpeg/ffprobe path (arm64 fallback, no MediaSDK) ─────────────────────────

struct FrameInfo {
    uint64_t pts_us;
    int      width;
    int      height;
};

static int count_video_streams(const std::string& path)
{
    const std::string cmd = "ffprobe -v error -select_streams v"
                            " -show_entries stream=index -of csv=p=0 \""
                            + path + "\" 2>/dev/null";
    int count = 0;
    std::istringstream ss(run_command(cmd));
    std::string line;
    while (std::getline(ss, line)) {
        if (!trim(line).empty()) ++count;
    }
    return count;
}

static bool get_stream_dimensions(const std::string& path, int& width, int& height,
                                  bool dual_fisheye)
{
    const std::string cmd = "ffprobe -v error -select_streams v:0"
                            " -show_entries stream=width,height -of csv=p=0 \""
                            + path + "\" 2>/dev/null";
    char sep = 0;
    if (sscanf(trim(run_command(cmd)).c_str(), "%d%c%d", &width, &sep, &height) != 3)
        return false;
    if (dual_fisheye) width *= 2;
    return true;
}

static std::vector<FrameInfo> get_frame_pts(const std::string& path, double fps_filter,
                                            bool dual_fisheye)
{
    int width = 0, height = 0;
    get_stream_dimensions(path, width, height, dual_fisheye);

    const std::string cmd = "ffprobe -v error -select_streams v:0"
                            " -show_entries packet=pts_time -of csv=p=0 \""
                            + path + "\" 2>/dev/null";
    std::vector<FrameInfo> frames;
    const double min_interval_s = (fps_filter > 0.0) ? 1.0 / fps_filter : 0.0;
    double next_keep_s = 0.0;
    std::istringstream ss(run_command(cmd));
    std::string line;
    while (std::getline(ss, line)) {
        line = trim(line);
        if (line.empty()) continue;
        double pts_s = 0.0;
        if (sscanf(line.c_str(), "%lf", &pts_s) != 1) continue;
        if (fps_filter > 0.0 && pts_s < next_keep_s) continue;
        frames.push_back({static_cast<uint64_t>(pts_s * 1e6), width, height});
        next_keep_s = pts_s + min_interval_s;
    }
    return frames;
}

static uint64_t process_file_ffmpeg(
    const std::string& mp4,
    uint64_t record_start_ns,
    double fps_filter, int jpeg_quality, bool no_stitch, double fov,
    storage::StorageManager& dst,
    uint32_t device_id, uint32_t& global_seq, uint64_t frame_offset)
{
    const int nstreams    = count_video_streams(mp4);
    const bool dual       = (!no_stitch && nstreams >= 2);
    if (dual)
        std::cout << "[ingest] ffmpeg: dual-fisheye (" << nstreams
                  << " streams) → v360 stitch (fov=" << fov << ")\n";
    else
        std::cout << "[ingest] ffmpeg: single stream\n";

    auto frames = get_frame_pts(mp4, fps_filter, dual);
    if (frames.empty()) {
        std::cerr << "[ingest] Warning: ffprobe returned no frames for " << mp4 << "\n";
        return 0;
    }
    std::cout << "[ingest] " << frames.size() << " frames  "
              << frames[0].width << "x" << frames[0].height << "\n";

    char tmp_tmpl[] = "/tmp/hera_ingest_XXXXXX";
    const char* tmp_dir = mkdtemp(tmp_tmpl);
    if (!tmp_dir) { std::cerr << "[ingest] mkdtemp failed\n"; return 0; }

    std::ostringstream ffcmd;
    ffcmd << "ffmpeg -i \"" << mp4 << "\"";
    if (dual) {
        std::ostringstream vf;
        vf << "[0:v:0][0:v:1]hstack=inputs=2[h]"
           << ";[h]v360=dfisheye:e:ih_fov=" << fov << ":iv_fov=" << fov;
        if (fps_filter > 0.0) vf << "[s];[s]fps=" << fps_filter;
        ffcmd << " -filter_complex \"" << vf.str() << "\"";
    } else {
        if (fps_filter > 0.0) ffcmd << " -vf fps=" << fps_filter;
    }
    ffcmd << " -q:v " << jpeg_quality
          << " \"" << tmp_dir << "/frame_%06d.jpg\" -y 2>/dev/null";

    std::cout << "[ingest] extracting frames via ffmpeg ...\n";
    [[maybe_unused]] int ff_rc = std::system(ffcmd.str().c_str());

    uint64_t written = 0;
    for (size_t fi = 0; fi < frames.size(); ++fi) {
        std::ostringstream p;
        p << tmp_dir << "/frame_" << std::setfill('0') << std::setw(6) << (fi + 1) << ".jpg";
        const auto jpeg = read_file(p.str());
        if (jpeg.empty()) {
            std::cerr << "[ingest] Warning: missing frame " << p.str() << "\n";
            continue;
        }
        const uint64_t ts_ns = record_start_ns + frames[fi].pts_us * 1000ULL;
        write_jpeg_packet(jpeg, ts_ns, static_cast<uint32_t>(frame_offset + fi),
                          frames[fi].width, frames[fi].height,
                          device_id, global_seq, dst);
        ++written;
    }

    [[maybe_unused]] int rm_rc = std::system(
        (std::string("rm -rf \"") + tmp_dir + "\"").c_str());
    return written;
}

// ── MediaSDK path (amd64 with libMediaSDK installed) ─────────────────────────

#ifdef HAVE_MEDIASDK
static uint64_t process_file_mediasdk(
    const std::string& src_path,
    uint64_t record_start_ns,
    double fps_filter, int out_w, int out_h,
    const std::string& model_dir,
    storage::StorageManager& dst,
    uint32_t device_id, uint32_t& global_seq, uint64_t frame_offset)
{
    ins::MediaFileInfo minfo{};
    if (!ins::GetMediaFileInfo({src_path}, minfo)) {
        std::cerr << "[ingest] MediaSDK: GetMediaFileInfo failed for " << src_path
                  << " — falling back to ffmpeg\n";
        return UINT64_MAX;  // signal to caller: fall back
    }

    const double src_fps = (minfo.fps > 0.0) ? minfo.fps : 30.0;
    const int64_t total_src_frames =
        static_cast<int64_t>(src_fps * static_cast<double>(minfo.duration_ms) / 1000.0);

    std::cout << "[ingest] MediaSDK: " << src_path << "\n"
              << "  source: " << minfo.width << "x" << minfo.height
              << " @ " << src_fps << "fps  " << minfo.duration_ms << "ms\n"
              << "  output: " << out_w << "x" << out_h << "\n";

    // Compute frame indices to export at requested fps
    std::vector<uint64_t> export_indices;
    if (fps_filter <= 0.0 || fps_filter >= src_fps) {
        export_indices.reserve(static_cast<size_t>(total_src_frames));
        for (int64_t i = 0; i < total_src_frames; ++i)
            export_indices.push_back(static_cast<uint64_t>(i));
    } else {
        const double step = src_fps / fps_filter;
        for (double idx = 0.0; idx < static_cast<double>(total_src_frames); idx += step)
            export_indices.push_back(static_cast<uint64_t>(idx));
    }
    if (export_indices.empty()) return 0;
    std::cout << "[ingest] MediaSDK: exporting " << export_indices.size() << " frames\n";

    // Temp dir for JPEG sequence
    char tmp_tmpl[] = "/tmp/hera_mediasdk_XXXXXX";
    const char* tmp_dir_c = mkdtemp(tmp_tmpl);
    if (!tmp_dir_c) { std::cerr << "[ingest] mkdtemp failed\n"; return 0; }
    const std::string tmp_dir(tmp_dir_c);

    if (!model_dir.empty()) ins::SetModelFileRootDir(model_dir);

    auto stitcher = std::make_shared<ins::VideoStitcher>();
    stitcher->SetInputPath({src_path});
    stitcher->SetImageSequenceInfo(tmp_dir, ins::IMAGE_TYPE::JPEG);
    stitcher->SetExportFrameSequence(export_indices);
    stitcher->SetOutputSize(out_w, out_h);
    stitcher->SetStitchType(ins::STITCH_TYPE::OPTFLOW);
    stitcher->EnableCuda(false);
    stitcher->SetSoftwareCodecUsage(true, true);
    stitcher->SetImageProcessingAccelType(ins::ImageProcessingAccel::kCPU);

    // StartStitch() is async — wait for completion callback
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false, has_error = false;
    stitcher->SetStitchProgressCallback([&](int progress, int error) {
        std::cerr << "\r[ingest] stitching " << progress << "%   " << std::flush;
        if (progress >= 100 || error != 0) {
            std::unique_lock<std::mutex> lk(mtx);
            if (error != 0) has_error = true;
            done = true;
            cv.notify_one();
        }
    });

    stitcher->StartStitch();
    { std::unique_lock<std::mutex> lk(mtx); cv.wait(lk, [&] { return done; }); }
    std::cerr << "\n";

    if (has_error) {
        std::cerr << "[ingest] MediaSDK: stitching failed\n";
        [[maybe_unused]] int rc = std::system(("rm -rf \"" + tmp_dir + "\"").c_str());
        return 0;
    }

    // Collect output JPEGs (sorted by filename to preserve frame order)
    const std::string ls_out =
        run_command("ls \"" + tmp_dir + "\"/*.jpg 2>/dev/null | sort");
    std::vector<std::string> jpeg_files;
    {
        std::istringstream ss(ls_out);
        std::string line;
        while (std::getline(ss, line)) {
            line = trim(line);
            if (!line.empty()) jpeg_files.push_back(line);
        }
    }

    if (jpeg_files.empty()) {
        std::cerr << "[ingest] MediaSDK: no output JPEGs found in " << tmp_dir << "\n";
        [[maybe_unused]] int rc = std::system(("rm -rf \"" + tmp_dir + "\"").c_str());
        return 0;
    }

    const size_t n = std::min(jpeg_files.size(), export_indices.size());
    if (jpeg_files.size() != export_indices.size()) {
        std::cerr << "[ingest] MediaSDK: expected " << export_indices.size()
                  << " frames, got " << jpeg_files.size() << " — using " << n << "\n";
    }

    uint64_t written = 0;
    for (size_t fi = 0; fi < n; ++fi) {
        const auto jpeg = read_file(jpeg_files[fi]);
        if (jpeg.empty()) {
            std::cerr << "[ingest] Warning: empty JPEG " << jpeg_files[fi] << "\n";
            continue;
        }
        // Timestamp: source frame index / fps → seconds since recording start
        const double pts_s  = static_cast<double>(export_indices[fi]) / src_fps;
        const uint64_t ts_ns = record_start_ns + static_cast<uint64_t>(pts_s * 1e9);
        write_jpeg_packet(jpeg, ts_ns,
                          static_cast<uint32_t>(frame_offset + fi),
                          static_cast<uint32_t>(out_w), static_cast<uint32_t>(out_h),
                          device_id, global_seq, dst);
        ++written;
    }

    [[maybe_unused]] int rm_rc =
        std::system(("rm -rf \"" + tmp_dir + "\"").c_str());
    std::cout << "[ingest] MediaSDK: wrote " << written << " frames\n";
    return written;
}
#endif  // HAVE_MEDIASDK

// ── Main ──────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    std::string session_json_path;
    std::string output_hera_path;
    std::string model_dir;
    double fps_filter  = 1.0;
    int jpeg_quality   = 3;
    [[maybe_unused]] int output_width   = 3840;
    [[maybe_unused]] int output_height  = 1920;
    bool no_stitch     = false;
    double fov         = 190.0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      ((arg == "--session"   || arg == "-s") && i + 1 < argc) session_json_path = argv[++i];
        else if ((arg == "--output"    || arg == "-o") && i + 1 < argc) output_hera_path  = argv[++i];
        else if ((arg == "--fps"       || arg == "-f") && i + 1 < argc) fps_filter        = std::stod(argv[++i]);
        else if ((arg == "--quality"   || arg == "-q") && i + 1 < argc) jpeg_quality      = std::stoi(argv[++i]);
        else if ((arg == "--width")                    && i + 1 < argc) output_width      = std::stoi(argv[++i]);
        else if ((arg == "--height")                   && i + 1 < argc) output_height     = std::stoi(argv[++i]);
        else if ((arg == "--model-dir")                && i + 1 < argc) model_dir         = argv[++i];
        else if  (arg == "--no-stitch")                                  no_stitch         = true;
        else if ((arg == "--fov")                      && i + 1 < argc) fov               = std::stod(argv[++i]);
        else if  (arg == "--help" || arg == "-h") {
            std::cout <<
                "Usage: hera-storage-ingest-insta-video\n"
                "  --session    <insta_session.json>\n"
                "  --output     <camera.hera>\n"
                " [--fps        <rate>]        default: 1\n"
                " [--quality    <1-31>]        JPEG quality (ffmpeg), default: 3\n"
                " [--width      <px>]          output width,  default: 3840\n"
                " [--height     <px>]          output height, default: 1920\n"
#ifdef HAVE_MEDIASDK
                " [--model-dir  <path>]        MediaSDK AI models dir\n"
#endif
                " [--fov        <degrees>]     per-lens FOV for ffmpeg v360, default: 190\n"
                " [--no-stitch]               skip stitching\n"
#ifdef HAVE_MEDIASDK
                "\nBuilt with MediaSDK — uses Insta360 VideoStitcher for stitching.\n"
#else
                "\nBuilt without MediaSDK — uses ffmpeg v360 filter for stitching.\n"
#endif
                ;
            return 0;
        }
    }

    if (session_json_path.empty() || output_hera_path.empty()) {
        std::cerr << "Error: --session and --output are required\n";
        return 1;
    }

    // ── Read session JSON ─────────────────────────────────────────────────────
    std::ifstream sfile(session_json_path);
    if (!sfile) { std::cerr << "Error: cannot open " << session_json_path << "\n"; return 1; }
    std::string json((std::istreambuf_iterator<char>(sfile)), std::istreambuf_iterator<char>());

    const uint64_t record_start_ns = json_extract_u64(json, "record_start_host_ns");
    if (record_start_ns == 0) {
        std::cerr << "Error: record_start_host_ns missing or zero\n"; return 1;
    }
    const auto mp4_files = json_extract_string_array(json, "mp4_files");
    if (mp4_files.empty()) {
        std::cerr << "Error: mp4_files array missing or empty\n"; return 1;
    }

    std::cout << "[ingest] record_start_ns = " << record_start_ns << "\n";
    for (const auto& p : mp4_files) std::cout << "[ingest] source: " << p << "\n";
#ifdef HAVE_MEDIASDK
    std::cout << "[ingest] backend: MediaSDK (libMediaSDK)\n";
#else
    std::cout << "[ingest] backend: ffmpeg v360\n";
#endif

    // ── Open output hera ──────────────────────────────────────────────────────
    auto dst = storage::StorageManager::open(output_hera_path, false);
    if (!dst) { std::cerr << "Error: cannot create " << output_hera_path << "\n"; return 1; }
    if (!dst->add_device("camera/insta/01", 128)) {
        std::cerr << "Error: add_device failed\n"; return 1;
    }
    dst->finish_add_device();

    constexpr uint32_t kDeviceId = 0;
    uint32_t global_seq  = 0;
    uint64_t total_frames = 0;

    // ── Process each source file ──────────────────────────────────────────────
#ifdef HAVE_MEDIASDK
    ins::InitEnv();
    ins::SetLogLevel(ins::InsLogLevel::ERR);
#endif

    for (const auto& mp4 : mp4_files) {
        std::cout << "[ingest] processing: " << mp4 << "\n";

#ifdef HAVE_MEDIASDK
        const uint64_t n = process_file_mediasdk(
            mp4, record_start_ns, fps_filter, output_width, output_height,
            model_dir, *dst, kDeviceId, global_seq, total_frames);
        if (n == UINT64_MAX) {
            std::cout << "[ingest] falling back to ffmpeg for " << mp4 << "\n";
            total_frames += process_file_ffmpeg(
                mp4, record_start_ns, fps_filter, jpeg_quality, no_stitch, fov,
                *dst, kDeviceId, global_seq, total_frames);
        } else {
            total_frames += n;
        }
#else
        total_frames += process_file_ffmpeg(
            mp4, record_start_ns, fps_filter, jpeg_quality, no_stitch, fov,
            *dst, kDeviceId, global_seq, total_frames);
#endif
    }

    dst->close();
    std::cout << "[ingest] done: " << total_frames << " frames -> " << output_hera_path << "\n";
    return 0;
}
