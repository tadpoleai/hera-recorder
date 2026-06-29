///
/// @file ingest_stream_hera.cpp
/// @brief Post-process a Stream-mode .hera file:
///        decode the H.264 video (0x0421 packets) and write JPEG frames as
///        InstaJpegFramePacket (0x0423) into a new .hera file.
///
/// In Stream mode the X4 outputs a dual-fisheye side-by-side H.264 stream
/// (e.g. 2880x1440: left half = front lens, right half = rear lens) over USB.
/// All frames are delivered as stream_index=0 in a single bitstream.
/// This tool decodes it to equirectangular JPEG at a chosen fps via the
/// ffmpeg v360=dfisheye:equirect filter and attaches the original host timestamps.
///
/// Usage:
///   hera-storage-ingest-stream-hera
///       --input   <stream_mode.hera>
///       --output  <jpeg_frames.hera>
///     [ --fps     <rate>    ]  frames per second to extract, default: 1
///     [ --quality <1-31>    ]  ffmpeg JPEG -q:v, default: 3 (lower = better)
///

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef HERA_COMPILE_IN_REPO
#include "storage/include/storage.hpp"
#include "device/include/device_data.hpp"
#else
#include <hera/storage/storage.hpp>
#include <hera/device/device_data.hpp>
#endif

using namespace wayz::hera;

// ── Packet type constants ─────────────────────────────────────────────────────

constexpr device::DeviceVendorType kInstaVendorType  = 0x0421;
constexpr device::DeviceDataType   kInstaVideoType   = 0x0421;  // raw H.264 frames
constexpr device::DeviceDataType   kInstaJpegType    = 0x0423;  // stitched JPEG frames

constexpr size_t kDeviceDataHeaderBytes = 24;  // length+device_id+vendor_type+msg_type+seq+ts

#pragma pack(push, 1)
struct VideoPacketHeader {
    int64_t  timestamp_device_ns;
    uint64_t timestamp_host_ns;
    uint8_t  stream_type;
    int32_t  stream_index;
    uint32_t payload_size;
};

struct InstaJpegFrameHeader {
    uint64_t timestamp_host_ns;
    uint32_t frame_index;
    uint32_t width;
    uint32_t height;
    uint32_t payload_size;
};
#pragma pack(pop)

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::vector<uint8_t> read_file(const std::string& path)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto size = f.tellg();
    if (size <= 0) return {};
    f.seekg(0);
    std::vector<uint8_t> buf(static_cast<size_t>(size));
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

// ── Step 1: scan input .hera, collect timestamps and write H.264 to temp file ─

struct FrameTimestamp {
    uint64_t host_ns;    // VideoPacketHeader.timestamp_host_ns
    size_t   byte_offset;  // byte offset in temp H.264 file (for future use)
};

static bool extract_h264_with_timestamps(
    const std::string& hera_path,
    const std::string& h264_path,
    std::vector<FrameTimestamp>& out_timestamps)
{
    auto src = storage::StorageManager::open(hera_path, true);
    if (!src) {
        std::cerr << "[ingest] cannot open input hera: " << hera_path << "\n";
        return false;
    }

    std::ofstream h264_out(h264_path, std::ios::binary);
    if (!h264_out) {
        std::cerr << "[ingest] cannot create temp file: " << h264_path << "\n";
        return false;
    }

    size_t byte_offset = 0;
    uint64_t video_packets = 0;

    while (auto data = src->read()) {
        if (!data || !data->is_type(kInstaVideoType)) continue;

        const uint8_t* raw = reinterpret_cast<const uint8_t*>(data.get());
        const uint8_t* payload = raw + kDeviceDataHeaderBytes;
        const size_t   payload_len = data->get_length() - kDeviceDataHeaderBytes;

        if (payload_len < sizeof(VideoPacketHeader)) continue;

        const auto* vh = reinterpret_cast<const VideoPacketHeader*>(payload);
        const uint8_t* video_data = payload + sizeof(VideoPacketHeader);
        const uint32_t video_size = vh->payload_size;

        if (video_size == 0 || video_size > payload_len - sizeof(VideoPacketHeader)) continue;

        // Only collect the primary stream (stream_index 0 = main stitched stream)
        if (vh->stream_index != 0) continue;

        out_timestamps.push_back({vh->timestamp_host_ns, byte_offset});
        h264_out.write(reinterpret_cast<const char*>(video_data), video_size);
        byte_offset += video_size;
        ++video_packets;
    }

    std::cout << "[ingest] read " << video_packets << " H.264 packets ("
              << byte_offset / 1024 << " KiB)\n";
    return video_packets > 0;
}

// ── Step 2: decode H.264 + stitch → JPEG sequence via ffmpeg ─────────────────

static bool decode_to_jpegs(const std::string& h264_path,
                             const std::string& out_dir,
                             double src_fps, double out_fps,
                             int quality,
                             int out_w, int out_h,
                             double fov_deg)
{
    std::ostringstream cmd;

    // Raw H.264 from .hera has no container timestamps and often starts with
    // P-frames before the first IDR/SPS.  The simple fps= filter freezes on
    // the first decodable frame when out_fps << src_fps.
    //
    // Fix: decode only I-frames (-skip_frame noref) and select every N-th one
    // so that the output is evenly spread across the recording.  For out_fps
    // close to or above 1 Hz the fps= filter works fine, so use it there.
    //
    // -err_detect ignore_err + -fflags +discardcorrupt: skip the corrupt
    // leading P-frames without aborting.
    const double step_f = src_fps / out_fps;  // source frames per output frame

    cmd << "ffmpeg -loglevel warning"
        << " -err_detect ignore_err -fflags +discardcorrupt"
        << " -analyzeduration 100M -probesize 100M";

    if (step_f >= 30.0) {
        // Low output fps: decode only I-frames and pick every step_i-th one.
        // I-frames occur every ~src_fps frames (1 per second for X4 stream).
        const int step_i = std::max(1, static_cast<int>(out_fps > 0
                                        ? 1.0 / out_fps + 0.5 : 1));
        cmd << " -skip_frame noref"
            << " -r " << src_fps
            << " -f h264 -i \"" << h264_path << "\""
            << " -vf \"select='eq(pict_type\\,I)*not(mod(n\\," << step_i << "))',setpts=N/(TB*" << src_fps << ")"
            << ",v360=dfisheye:equirect"
            << ":ih_fov=" << fov_deg << ":iv_fov=" << fov_deg
            << ":w=" << out_w << ":h=" << out_h << "\""
            << " -vsync vfr";
    } else {
        // High output fps: standard fps= filter works reliably.
        cmd << " -r " << src_fps
            << " -f h264 -i \"" << h264_path << "\""
            << " -vf \"fps=" << out_fps
            << ",v360=dfisheye:equirect"
            << ":ih_fov=" << fov_deg << ":iv_fov=" << fov_deg
            << ":w=" << out_w << ":h=" << out_h << "\""
            << " -vsync cfr";
    }

    cmd << " -q:v " << quality
        << " \"" << out_dir << "/frame_%06d.jpg\""
        << " -y";

    std::cout << "[ingest] decoding+stitching: " << cmd.str() << "\n";
    return std::system(cmd.str().c_str()) == 0;  // NOLINT
}

// ── Step 3: write InstaJpegFramePackets to output .hera ──────────────────────

static void write_jpeg_packet(
    const std::vector<uint8_t>& jpeg,
    uint64_t ts_ns, uint32_t frame_idx,
    uint32_t width, uint32_t height,
    uint32_t device_id, uint32_t& seq,
    storage::StorageManager& dst)
{
    const uint32_t total = static_cast<uint32_t>(
        kDeviceDataHeaderBytes + sizeof(InstaJpegFrameHeader) + jpeg.size());
    auto pkt = device::data::DeviceData::create(
        total, device_id, kInstaVendorType, kInstaJpegType, seq++);
    if (!pkt) { std::cerr << "[ingest] DeviceData::create failed\n"; return; }

    auto* hdr = reinterpret_cast<InstaJpegFrameHeader*>(
        reinterpret_cast<uint8_t*>(pkt.get()) + kDeviceDataHeaderBytes);
    hdr->timestamp_host_ns = ts_ns;
    hdr->frame_index       = frame_idx;
    hdr->width             = width;
    hdr->height            = height;
    hdr->payload_size      = static_cast<uint32_t>(jpeg.size());
    std::memcpy(hdr + 1, jpeg.data(), jpeg.size());
    dst.add_data(device_id, pkt, true);
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    std::string input_hera;
    std::string output_hera;
    double out_fps   = 1.0;
    int    quality   = 3;
    double src_fps   = 30.0;   // X4 USB streaming rate
    int    out_w     = 3840;   // equirectangular output width
    int    out_h     = 1920;   // equirectangular output height
    double fov_deg   = 190.0;  // X4 per-lens FOV

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if      ((arg == "--input"   || arg == "-i") && i + 1 < argc) input_hera  = argv[++i];
        else if ((arg == "--output"  || arg == "-o") && i + 1 < argc) output_hera = argv[++i];
        else if ((arg == "--fps"     || arg == "-f") && i + 1 < argc) out_fps     = std::stod(argv[++i]);
        else if ((arg == "--quality" || arg == "-q") && i + 1 < argc) quality     = std::stoi(argv[++i]);
        else if  (arg == "--src-fps"                 && i + 1 < argc) src_fps     = std::stod(argv[++i]);
        else if  (arg == "--width"                   && i + 1 < argc) out_w       = std::stoi(argv[++i]);
        else if  (arg == "--height"                  && i + 1 < argc) out_h       = std::stoi(argv[++i]);
        else if  (arg == "--fov"                     && i + 1 < argc) fov_deg     = std::stod(argv[++i]);
        else if  (arg == "--help"    || arg == "-h") {
            std::cout <<
                "Usage: hera-storage-ingest-stream-hera\n"
                "  --input    <stream_mode.hera>   source from Stream-mode capture\n"
                "  --output   <frames.hera>        output with InstaJpegFramePackets\n"
                " [--fps      <rate>]              output fps, default: 1\n"
                " [--quality  <1-31>]              JPEG quality, default: 3\n"
                " [--src-fps  <rate>]              source H.264 fps, default: 30\n"
                " [--width    <px>]                equirectangular output width,  default: 3840\n"
                " [--height   <px>]                equirectangular output height, default: 1920\n"
                " [--fov      <deg>]               per-lens FOV for v360, default: 190\n";
            return 0;
        }
    }

    if (input_hera.empty() || output_hera.empty()) {
        std::cerr << "Error: --input and --output are required\n";
        return 1;
    }

    // ── Step 1: extract H.264 and timestamps ─────────────────────────────────
    char tmp_tmpl[] = "/tmp/hera_stream_XXXXXX";
    const char* tmp_dir_c = mkdtemp(tmp_tmpl);
    if (!tmp_dir_c) { std::cerr << "mkdtemp failed\n"; return 1; }
    const std::string tmp_dir(tmp_dir_c);
    const std::string h264_path = tmp_dir + "/stream.h264";

    std::vector<FrameTimestamp> src_timestamps;
    if (!extract_h264_with_timestamps(input_hera, h264_path, src_timestamps)) {
        std::cerr << "Error: no video packets found in " << input_hera << "\n";
        [[maybe_unused]] int rc = std::system(("rm -rf \"" + tmp_dir + "\"").c_str());
        return 1;
    }

    // ── Step 2: decode + stitch to JPEG frames ───────────────────────────────
    if (!decode_to_jpegs(h264_path, tmp_dir, src_fps, out_fps, quality,
                         out_w, out_h, fov_deg)) {
        std::cerr << "Warning: ffmpeg exited non-zero (may still have produced frames)\n";
    }

    // ── Step 3: collect output JPEGs (sorted) ────────────────────────────────
    const size_t n_src = src_timestamps.size();

    std::vector<std::string> jpegs;
    for (size_t fi = 1; ; ++fi) {
        std::ostringstream p;
        p << tmp_dir << "/frame_" << std::setfill('0') << std::setw(6) << fi << ".jpg";
        if (read_file(p.str()).empty()) break;
        jpegs.push_back(p.str());
    }

    if (jpegs.empty()) {
        std::cerr << "Error: ffmpeg produced no JPEG frames\n";
        [[maybe_unused]] int rc = std::system(("rm -rf \"" + tmp_dir + "\"").c_str());
        return 1;
    }
    std::cout << "[ingest] stitched " << jpegs.size() << " JPEG frames ("
              << out_w << "x" << out_h << ")\n";

    // ── Step 4: open output hera and write packets ────────────────────────────
    auto dst = storage::StorageManager::open(output_hera, false);
    if (!dst) { std::cerr << "Error: cannot create " << output_hera << "\n"; return 1; }
    if (!dst->add_device("camera/insta/01", 128)) {
        std::cerr << "Error: add_device failed\n"; return 1;
    }
    dst->finish_add_device();

    constexpr uint32_t kDeviceId = 0;
    uint32_t seq = 0;

    const size_t n_out = jpegs.size();
    for (size_t fi = 0; fi < n_out; ++fi) {
        const auto jpeg = read_file(jpegs[fi]);
        if (jpeg.empty()) {
            std::cerr << "[ingest] Warning: empty frame " << jpegs[fi] << "\n";
            continue;
        }

        // Linearly interpolate source timestamp across the full recording span.
        // Each SDK packet may contain multiple H.264 frames, so a fixed step
        // based on src_fps/out_fps would be inaccurate for longer recordings.
        const double ratio = (n_out > 1) ? static_cast<double>(fi) / (n_out - 1) : 0.0;
        const size_t src_idx = std::min(
            static_cast<size_t>(ratio * (n_src - 1) + 0.5), n_src - 1);
        const uint64_t ts_ns = src_timestamps[src_idx].host_ns;

        write_jpeg_packet(jpeg, ts_ns, static_cast<uint32_t>(fi),
                          static_cast<uint32_t>(out_w),
                          static_cast<uint32_t>(out_h),
                          kDeviceId, seq, *dst);
    }

    dst->close();
    [[maybe_unused]] int rm_rc = std::system(("rm -rf \"" + tmp_dir + "\"").c_str());

    std::cout << "[ingest] done: " << jpegs.size() << " frames -> " << output_hera << "\n";
    return 0;
}
