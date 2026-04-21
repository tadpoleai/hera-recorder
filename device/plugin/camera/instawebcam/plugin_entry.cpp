///
/// @file plugin_entry.cpp
/// @brief Insta webcam phase-1 plugin: V4L2 raw frame capture
///

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "plugin_common.hpp"
#include "plugin_param.hpp"

#ifdef WITH_DRIVER
#include <cerrno>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace wayz {
namespace hera {
namespace device {
namespace camera {
namespace instawebcam {

HERA_PLUGIN_DEFINE_START("camera/instawebcam", 0x0431, 128)

#include "plugin_data.hpp"

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

struct RawSample {
    uint64_t host_ts_ns;
    uint64_t frame_index;
    uint32_t width;
    uint32_t height;
    uint32_t fourcc;
    std::vector<uint8_t> payload;
};

struct RuntimeState {
    std::mutex mutex;
    std::condition_variable cv;
    std::deque<RawSample> queue;
    bool stopped = false;

    std::atomic<uint64_t> rx_video_packets{0};
    std::atomic<uint64_t> out_video_packets{0};
    uint64_t last_stats_log_ns = 0;
};

std::shared_ptr<RuntimeState> runtime_;
std::thread capture_thread_;
std::atomic<bool> capture_running_{false};
int camera_fd_ = -1;
bool use_mmap_io_ = false;
bool streaming_on_ = false;
uint32_t current_width_ = 0;
uint32_t current_height_ = 0;
uint32_t current_fourcc_ = 0;
uint64_t frame_seq_ = 0;

std::vector<std::pair<void*, size_t>> mapped_buffers_;

static uint32_t pixel_format_to_fourcc(const PixelFormat fmt)
{
    if (fmt._to_integral() == static_cast<int32_t>(PixelFormat::YUYV)) {
        return V4L2_PIX_FMT_YUYV;
    }
    return V4L2_PIX_FMT_MJPEG;
}

static std::string fourcc_to_string(uint32_t fourcc)
{
    std::string out(4, ' ');
    out[0] = static_cast<char>(fourcc & 0xFF);
    out[1] = static_cast<char>((fourcc >> 8) & 0xFF);
    out[2] = static_cast<char>((fourcc >> 16) & 0xFF);
    out[3] = static_cast<char>((fourcc >> 24) & 0xFF);
    return out;
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
    log::info << "InstaWebcam: rx(video)=" << rt->rx_video_packets.load()
              << ", out(video)=" << rt->out_video_packets.load() << log::endl;
}

static bool set_stream_param(int fd, int fps)
{
    struct v4l2_streamparm parm;
    memset(&parm, 0, sizeof(parm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = static_cast<uint32_t>(std::max(1, fps));
    return ioctl(fd, VIDIOC_S_PARM, &parm) == 0;
}

static int xioctl(int fd, unsigned long request, void* arg)
{
    int ret;
    do {
        ret = ioctl(fd, request, arg);
    } while (ret == -1 && errno == EINTR);
    return ret;
}

#endif

HERA_PLUGIN_DEFINE_END

#ifdef WITH_DRIVER

namespace {

int xioctl_local(int fd, unsigned long request, void* arg)
{
    int ret;
    do {
        ret = ioctl(fd, request, arg);
    } while (ret == -1 && errno == EINTR);
    return ret;
}

void release_mmap_buffers(std::vector<std::pair<void*, size_t>>& buffers)
{
    for (auto& b : buffers) {
        if (b.first && b.second > 0) {
            munmap(b.first, b.second);
            b.first = nullptr;
            b.second = 0;
        }
    }
    buffers.clear();
}

bool setup_mmap_io(int fd, std::vector<std::pair<void*, size_t>>& buffers)
{
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (xioctl_local(fd, VIDIOC_REQBUFS, &req) != 0 || req.count < 2) {
        return false;
    }

    buffers.clear();
    buffers.resize(req.count);

    for (uint32_t i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (xioctl_local(fd, VIDIOC_QUERYBUF, &buf) != 0) {
            release_mmap_buffers(buffers);
            return false;
        }

        void* p = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (p == MAP_FAILED) {
            release_mmap_buffers(buffers);
            return false;
        }

        buffers[i].first = p;
        buffers[i].second = static_cast<size_t>(buf.length);
    }

    for (uint32_t i = 0; i < buffers.size(); ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (xioctl_local(fd, VIDIOC_QBUF, &buf) != 0) {
            release_mmap_buffers(buffers);
            return false;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl_local(fd, VIDIOC_STREAMON, &type) != 0) {
        release_mmap_buffers(buffers);
        return false;
    }

    return true;
}

}  // namespace

HeraErrno DevicePlugin::connect()
{
    runtime_ = std::make_shared<RuntimeState>();

    const std::string device_path = local_parameters_.get_DevicePath();
    camera_fd_ = open(device_path.c_str(), O_RDWR);
    if (camera_fd_ < 0) {
        return handle_error(HeraErrno::CanNotOpenCamera, "InstaWebcam: can not open V4L2 device");
    }

    use_mmap_io_ = false;
    streaming_on_ = false;
    release_mmap_buffers(mapped_buffers_);

    struct v4l2_capability cap;
    memset(&cap, 0, sizeof(cap));
    if (ioctl(camera_fd_, VIDIOC_QUERYCAP, &cap) != 0) {
        close(camera_fd_);
        camera_fd_ = -1;
        return handle_error(HeraErrno::CanNotOpenCamera, "InstaWebcam: VIDIOC_QUERYCAP failed");
    }

    const uint32_t effective_caps =
        (cap.capabilities & V4L2_CAP_DEVICE_CAPS) ? cap.device_caps : cap.capabilities;

    if ((effective_caps & V4L2_CAP_VIDEO_CAPTURE) == 0) {
        close(camera_fd_);
        camera_fd_ = -1;
        return handle_error(HeraErrno::CanNotOpenCamera, "InstaWebcam: not a V4L2 capture device");
    }

    const bool supports_read = (effective_caps & V4L2_CAP_READWRITE) != 0;
    const bool supports_streaming = (effective_caps & V4L2_CAP_STREAMING) != 0;

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = static_cast<uint32_t>(local_parameters_.get_Width());
    fmt.fmt.pix.height = static_cast<uint32_t>(local_parameters_.get_Height());
    fmt.fmt.pix.pixelformat = pixel_format_to_fourcc(local_parameters_.get_PixelFormat());
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    if (ioctl(camera_fd_, VIDIOC_S_FMT, &fmt) != 0) {
        close(camera_fd_);
        camera_fd_ = -1;
        return handle_error(HeraErrno::CanNotOpenCamera, "InstaWebcam: VIDIOC_S_FMT failed");
    }

    current_width_ = fmt.fmt.pix.width;
    current_height_ = fmt.fmt.pix.height;
    current_fourcc_ = fmt.fmt.pix.pixelformat;

    if (!supports_read) {
        if (!supports_streaming) {
            close(camera_fd_);
            camera_fd_ = -1;
            return handle_error(HeraErrno::CanNotOpenCamera,
                                "InstaWebcam: device supports neither V4L2 read() nor streaming I/O");
        }

        if (!setup_mmap_io(camera_fd_, mapped_buffers_)) {
            close(camera_fd_);
            camera_fd_ = -1;
            return handle_error(HeraErrno::CanNotOpenCamera,
                                "InstaWebcam: MMAP streaming setup failed on this /dev/videoX");
        }

        use_mmap_io_ = true;
        streaming_on_ = true;
    }

    (void)set_stream_param(camera_fd_, local_parameters_.get_Fps());

    const uint32_t read_buffer_bytes = static_cast<uint32_t>(std::max<int32_t>(
        65536, std::max(local_parameters_.get_ReadBufferBytes(), static_cast<int32_t>(fmt.fmt.pix.sizeimage))));

    log::info << "InstaWebcam: opened " << device_path << ", io=" << (use_mmap_io_ ? "mmap" : "read")
              << ", fmt=" << fourcc_to_string(current_fourcc_)
              << ", size=" << current_width_ << "x" << current_height_ << ", read_buffer="
              << read_buffer_bytes << log::endl;

    capture_running_.store(true);
    frame_seq_ = 0;
    capture_thread_ = std::thread([this, read_buffer_bytes]() {
        auto rt = runtime_;
        if (!rt) {
            return;
        }

        std::vector<uint8_t> buffer(read_buffer_bytes);
        uint64_t last_error_log_ns = 0;
        int last_errno = 0;
        uint32_t suppressed_count = 0;
        while (capture_running_.load()) {
            ssize_t n = -1;
            uint64_t host_ts_ns = static_cast<uint64_t>(time::Timestamp::now());

            if (use_mmap_io_) {
                fd_set fds;
                FD_ZERO(&fds);
                FD_SET(camera_fd_, &fds);
                struct timeval tv;
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                const int ready = select(camera_fd_ + 1, &fds, nullptr, nullptr, &tv);
                if (ready <= 0) {
                    if (ready < 0 && errno != EINTR) {
                        const int err = errno;
                        const uint64_t now_ns = static_cast<uint64_t>(time::Timestamp::now());
                        if (last_error_log_ns == 0 ||
                            now_ns - last_error_log_ns >= static_cast<uint64_t>(time::OneSecond) ||
                            err != last_errno) {
                            log::warn << "InstaWebcam: V4L2 select failed errno=" << err << log::endl;
                            last_error_log_ns = now_ns;
                            last_errno = err;
                        }
                    }
                    continue;
                }

                struct v4l2_buffer vbuf;
                memset(&vbuf, 0, sizeof(vbuf));
                vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                vbuf.memory = V4L2_MEMORY_MMAP;
                if (xioctl(camera_fd_, VIDIOC_DQBUF, &vbuf) != 0) {
                    if (errno == EAGAIN) {
                        continue;
                    }
                    n = -1;
                } else {
                    if (vbuf.index >= mapped_buffers_.size()) {
                        n = -1;
                        errno = EINVAL;
                    } else {
                        const size_t bytes = std::min<size_t>(vbuf.bytesused, mapped_buffers_[vbuf.index].second);
                        if (bytes > 0) {
                            n = static_cast<ssize_t>(bytes);
                            buffer.resize(bytes);
                            memcpy(buffer.data(), mapped_buffers_[vbuf.index].first, bytes);
                        } else {
                            n = 0;
                        }
                    }

                    if (xioctl(camera_fd_, VIDIOC_QBUF, &vbuf) != 0) {
                        n = -1;
                    }
                    host_ts_ns = static_cast<uint64_t>(time::Timestamp::now());
                }
            } else {
                n = read(camera_fd_, buffer.data(), buffer.size());
            }

            if (n < 0) {
                if (errno == EINTR || errno == EAGAIN) {
                    continue;
                }

                const int err = errno;
                const uint64_t now_ns = static_cast<uint64_t>(time::Timestamp::now());
                if (last_error_log_ns == 0 || now_ns - last_error_log_ns >= static_cast<uint64_t>(time::OneSecond) ||
                    err != last_errno) {
                    if (suppressed_count > 0) {
                        log::warn << "InstaWebcam: suppressed repeated read errors count=" << suppressed_count
                                  << log::endl;
                        suppressed_count = 0;
                    }
                    log::warn << "InstaWebcam: V4L2 read failed errno=" << err << log::endl;
                    last_error_log_ns = now_ns;
                    last_errno = err;
                } else {
                    ++suppressed_count;
                }

                if (err == EINVAL || err == ENODEV || err == EIO) {
                    log::error << "InstaWebcam: fatal V4L2 read error, stopping capture thread" << log::endl;
                    capture_running_.store(false);
                    std::lock_guard<std::mutex> lock(rt->mutex);
                    rt->stopped = true;
                    rt->cv.notify_all();
                    break;
                }
                continue;
            }

            if (n == 0) {
                continue;
            }

            RawSample sample;
            sample.host_ts_ns = host_ts_ns;
            sample.frame_index = frame_seq_++;
            sample.width = current_width_;
            sample.height = current_height_;
            sample.fourcc = current_fourcc_;
            sample.payload.resize(static_cast<size_t>(n));
            memcpy(sample.payload.data(), buffer.data(), sample.payload.size());

            {
                std::lock_guard<std::mutex> lock(rt->mutex);
                if (rt->stopped) {
                    break;
                }
                rt->queue.emplace_back(std::move(sample));
                rt->cv.notify_one();
            }

            rt->rx_video_packets.fetch_add(1);
        }
    });

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

    capture_running_.store(false);
    if (streaming_on_ && camera_fd_ >= 0) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        (void)xioctl_local(camera_fd_, VIDIOC_STREAMOFF, &type);
        streaming_on_ = false;
    }

    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }

    release_mmap_buffers(mapped_buffers_);

    if (camera_fd_ >= 0) {
        close(camera_fd_);
        camera_fd_ = -1;
    }

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
    }

    const auto length = static_cast<uint32_t>(sizeof(InstaWebcamVideoPacket) + sample.payload.size());
    auto data = InstaWebcamVideoPacket::create(length, id_, sequence_++);
    auto raw = static_cast<InstaWebcamVideoPacket*>(data.get());

    raw->timestamp_host_ns = sample.host_ts_ns;
    raw->frame_index = sample.frame_index;
    raw->width = sample.width;
    raw->height = sample.height;
    raw->fourcc = sample.fourcc;
    raw->payload_size = sample.payload.size();
    if (!sample.payload.empty()) {
        memcpy(raw->payload, sample.payload.data(), sample.payload.size());
    }

    local_rt->out_video_packets.fetch_add(1);
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
    return data::SensorData::broken_data();
}

}  // namespace instawebcam
}  // namespace camera
}  // namespace device
}  // namespace hera
}  // namespace wayz
