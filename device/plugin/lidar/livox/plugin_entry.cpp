///
/// @file plugin_entry.cpp
/// @brief Livox Mid360 phase-1 plugin: realtime raw packet + imu capture
///

#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "data/imu_data.hpp"
#include "plugin_common.hpp"
#include "plugin_param.hpp"

#ifdef WITH_DRIVER
#include "livox_lidar_api.h"
#include "livox_lidar_def.h"
#endif

namespace wayz {
namespace hera {
namespace device {
namespace lidar {
namespace livox {

HERA_PLUGIN_DEFINE_START("lidar/livox", 0x0521, 128)

#include "plugin_data.hpp"

#ifdef WITH_DRIVER
HERA_PLUGIN_DEFINE_FUNCTIONS

struct RawSample {
    bool is_imu;
    uint64_t device_ts_ns;
    uint64_t host_rx_ns;
    uint32_t handle;
    uint8_t dev_type;
    uint8_t data_type;
    uint8_t time_type;
    uint8_t frame_cnt;
    uint16_t dot_num;
    uint16_t packet_length;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float acc_x;
    float acc_y;
    float acc_z;
    std::vector<uint8_t> payload;
};

struct RuntimeState {
    std::mutex mutex;
    std::condition_variable cv;
    std::deque<RawSample> queue;
    bool stopped = false;
    bool enable_imu = true;
    std::string sn_filter;
    std::string bound_sn;
    uint32_t bound_handle = 0;
};

static std::mutex g_sdk_mutex;
static int g_sdk_ref_count;
static std::string g_sdk_config_path;
static bool g_sdk_running;

static std::mutex g_bind_mutex;
static std::unordered_map<uint32_t, std::weak_ptr<RuntimeState>> g_handle_runtime_map;
static std::vector<std::weak_ptr<RuntimeState>> g_unbound_runtimes;

static uint64_t parse_timestamp_ns(const LivoxLidarEthernetPacket* pkt)
{
    uint64_t raw = 0;
    static_assert(sizeof(raw) == 8, "uint64_t must be 8 bytes");
    memcpy(&raw, pkt->timestamp, sizeof(raw));
    // Phase-1: preserve raw semantic and pass through as device timestamp.
    return raw;
}

static constexpr uint16_t kLivoxHeaderSize = offsetof(LivoxLidarEthernetPacket, data);

static std::shared_ptr<RuntimeState> find_runtime(uint32_t handle)
{
    std::lock_guard<std::mutex> lock(g_bind_mutex);
    auto it = g_handle_runtime_map.find(handle);
    if (it == g_handle_runtime_map.end()) {
        return nullptr;
    }
    return it->second.lock();
}

static void enqueue_sample(const std::shared_ptr<RuntimeState>& rt, RawSample&& sample)
{
    std::lock_guard<std::mutex> lock(rt->mutex);
    if (rt->stopped) {
        return;
    }
    rt->queue.emplace_back(std::move(sample));
    rt->cv.notify_one();
}

static void bind_handle_if_needed(uint32_t handle, const char* sn)
{
    std::lock_guard<std::mutex> lock(g_bind_mutex);

    auto mapped = g_handle_runtime_map.find(handle);
    if (mapped != g_handle_runtime_map.end() && !mapped->second.expired()) {
        return;
    }

    std::vector<std::weak_ptr<RuntimeState>> remaining;
    remaining.reserve(g_unbound_runtimes.size());

    std::shared_ptr<RuntimeState> chosen = nullptr;
    for (auto&& weak_rt : g_unbound_runtimes) {
        auto rt = weak_rt.lock();
        if (!rt) {
            continue;
        }
        if (rt->bound_handle != 0) {
            continue;
        }

        if (!chosen) {
            if (!rt->sn_filter.empty()) {
                if (sn && rt->sn_filter == sn) {
                    chosen = rt;
                    continue;
                }
            } else {
                chosen = rt;
                continue;
            }
        }
        remaining.emplace_back(rt);
    }

    if (chosen) {
        chosen->bound_handle = handle;
        chosen->bound_sn = sn ? sn : "";
        g_handle_runtime_map[handle] = chosen;
    }

    g_unbound_runtimes.swap(remaining);
}

static void PointCloudCallback(uint32_t handle,
                               const uint8_t dev_type,
                               LivoxLidarEthernetPacket* data,
                               void* client_data)
{
    (void)client_data;
    if (!data) {
        return;
    }

    auto rt = find_runtime(handle);
    if (!rt) {
        return;
    }

    RawSample sample;
    sample.is_imu = false;
    sample.device_ts_ns = parse_timestamp_ns(data);
    sample.host_rx_ns = static_cast<uint64_t>(time::Timestamp::now());
    sample.handle = handle;
    sample.dev_type = dev_type;
    sample.data_type = data->data_type;
    sample.time_type = data->time_type;
    sample.frame_cnt = data->frame_cnt;
    sample.dot_num = data->dot_num;
    sample.packet_length = data->length;

    const uint16_t payload_size = data->length > kLivoxHeaderSize
                                          ? static_cast<uint16_t>(data->length - kLivoxHeaderSize)
                                          : 0;
    sample.payload.resize(payload_size);
    if (payload_size > 0) {
        memcpy(sample.payload.data(), data->data, payload_size);
    }

    enqueue_sample(rt, std::move(sample));
}

static void ImuDataCallback(uint32_t handle,
                            const uint8_t dev_type,
                            LivoxLidarEthernetPacket* data,
                            void* client_data)
{
    (void)client_data;
    if (!data) {
        return;
    }

    auto rt = find_runtime(handle);
    if (!rt || !rt->enable_imu) {
        return;
    }

    RawSample sample;
    sample.is_imu = true;
    sample.device_ts_ns = parse_timestamp_ns(data);
    sample.host_rx_ns = static_cast<uint64_t>(time::Timestamp::now());
    sample.handle = handle;
    sample.dev_type = dev_type;
    sample.data_type = data->data_type;
    sample.time_type = data->time_type;
    sample.frame_cnt = data->frame_cnt;
    sample.dot_num = data->dot_num;
    sample.packet_length = data->length;

    const uint16_t payload_size = data->length > kLivoxHeaderSize
                                          ? static_cast<uint16_t>(data->length - kLivoxHeaderSize)
                                          : 0;
    sample.payload.resize(payload_size);
    if (payload_size > 0) {
        memcpy(sample.payload.data(), data->data, payload_size);
    }

    if (payload_size >= sizeof(LivoxLidarImuRawPoint)) {
        auto imu = reinterpret_cast<const LivoxLidarImuRawPoint*>(data->data);
        sample.gyro_x = imu->gyro_x;
        sample.gyro_y = imu->gyro_y;
        sample.gyro_z = imu->gyro_z;
        sample.acc_x = imu->acc_x;
        sample.acc_y = imu->acc_y;
        sample.acc_z = imu->acc_z;
    }

    enqueue_sample(rt, std::move(sample));
}

static void LidarInfoChangeCallback(uint32_t handle, const LivoxLidarInfo* info, void* client_data)
{
    (void)client_data;
    if (!info) {
        return;
    }
    bind_handle_if_needed(handle, info->sn);
}

std::shared_ptr<RuntimeState> runtime_;
#endif

HERA_PLUGIN_DEFINE_END

#ifdef WITH_DRIVER

std::mutex DevicePlugin::g_sdk_mutex;
int DevicePlugin::g_sdk_ref_count = 0;
std::string DevicePlugin::g_sdk_config_path;
bool DevicePlugin::g_sdk_running = false;

std::mutex DevicePlugin::g_bind_mutex;
std::unordered_map<uint32_t, std::weak_ptr<DevicePlugin::RuntimeState>> DevicePlugin::g_handle_runtime_map;
std::vector<std::weak_ptr<DevicePlugin::RuntimeState>> DevicePlugin::g_unbound_runtimes;

HeraErrno DevicePlugin::connect()
{
    auto config_path = local_parameters_.get_ConfigPath();
    if (config_path.empty()) {
        return handle_error(HeraErrno::InvalidParameterValue, "ConfigPath is empty");
    }

    runtime_ = std::make_shared<RuntimeState>();
    runtime_->enable_imu = local_parameters_.get_EnableImu();
    runtime_->sn_filter = local_parameters_.get_DeviceSn();

    {
        std::lock_guard<std::mutex> lock(g_sdk_mutex);
        if (g_sdk_ref_count == 0) {
            if (!LivoxLidarSdkInit(config_path.c_str())) {
                return handle_error(HeraErrno::CanNotOpenEthernetDevice, "LivoxLidarSdkInit failed");
            }

            SetLivoxLidarPointCloudCallBack(PointCloudCallback, nullptr);
            SetLivoxLidarImuDataCallback(ImuDataCallback, nullptr);
            SetLivoxLidarInfoChangeCallback(LidarInfoChangeCallback, nullptr);

            if (!LivoxLidarSdkStart()) {
                LivoxLidarSdkUninit();
                return handle_error(HeraErrno::CanNotOpenEthernetDevice, "LivoxLidarSdkStart failed");
            }
            g_sdk_running = true;
            g_sdk_config_path = config_path;
        } else if (g_sdk_config_path != config_path) {
            return handle_error(HeraErrno::InvalidParameterValue,
                                "ConfigPath mismatch with running Livox SDK instance");
        }
        ++g_sdk_ref_count;
    }

    {
        std::lock_guard<std::mutex> lock(g_bind_mutex);
        g_unbound_runtimes.emplace_back(runtime_);
    }

    return HeraErrno::Success;
}

void DevicePlugin::disconnect()
{
    auto local_rt = runtime_;
    if (local_rt) {
        {
            std::lock_guard<std::mutex> lock(local_rt->mutex);
            local_rt->stopped = true;
            local_rt->queue.clear();
            local_rt->cv.notify_all();
        }

        std::lock_guard<std::mutex> lock(g_bind_mutex);
        if (local_rt->bound_handle != 0) {
            g_handle_runtime_map.erase(local_rt->bound_handle);
        }
    }

    {
        std::lock_guard<std::mutex> lock(g_sdk_mutex);
        if (g_sdk_ref_count > 0) {
            --g_sdk_ref_count;
        }
        if (g_sdk_ref_count == 0 && g_sdk_running) {
            LivoxLidarSdkUninit();
            g_sdk_running = false;
            g_sdk_config_path.clear();

            std::lock_guard<std::mutex> bind_lock(g_bind_mutex);
            g_handle_runtime_map.clear();
            g_unbound_runtimes.clear();
        }
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
            return nullptr;
        }
        if (local_rt->stopped || local_rt->queue.empty()) {
            return nullptr;
        }

        sample = std::move(local_rt->queue.front());
        local_rt->queue.pop_front();
    }

    if (sample.is_imu) {
        auto length = static_cast<uint32_t>(sizeof(LivoxImuPacket) + sample.payload.size());
        auto data = LivoxImuPacket::create(length, id_, sequence_++);
        auto raw = static_cast<LivoxImuPacket*>(data.get());

        raw->timestamp_device_ns = sample.device_ts_ns;
        raw->timestamp_host_ns = sample.host_rx_ns;
        raw->handle = sample.handle;
        raw->livox_dev_type = sample.dev_type;
        raw->livox_time_type = sample.time_type;
        raw->gyro_x = sample.gyro_x;
        raw->gyro_y = sample.gyro_y;
        raw->gyro_z = sample.gyro_z;
        raw->acc_x = sample.acc_x;
        raw->acc_y = sample.acc_y;
        raw->acc_z = sample.acc_z;
        raw->payload_size = sample.payload.size();
        if (!sample.payload.empty()) {
            memcpy(raw->payload, sample.payload.data(), sample.payload.size());
        }
        return data;
    }

    data::DeviceDataPtr data{nullptr};
    const auto length = static_cast<uint32_t>(sizeof(LivoxPacket) + sample.payload.size());

    switch (local_parameters_.get_SyncType()) {
    case SyncType::Full:
        data = LivoxPacketFullSynced::create(length, id_, sequence_++);
        break;
    case SyncType::Local:
        data = LivoxPacketLocalSynced::create(length, id_, sequence_++);
        break;
    case SyncType::Disabled:
        data = LivoxPacketUnSynced::create(length, id_, sequence_++);
        break;
    }

    auto raw = static_cast<LivoxPacket*>(data.get());
    raw->timestamp_device_ns = sample.device_ts_ns;
    raw->timestamp_host_ns = sample.host_rx_ns;
    raw->handle = sample.handle;
    raw->livox_dev_type = sample.dev_type;
    raw->livox_data_type = sample.data_type;
    raw->livox_time_type = sample.time_type;
    raw->frame_cnt = sample.frame_cnt;
    raw->dot_num = sample.dot_num;
    raw->packet_length = sample.packet_length;
    raw->payload_size = sample.payload.size();
    if (!sample.payload.empty()) {
        memcpy(raw->payload, sample.payload.data(), sample.payload.size());
    }

    return data;
}

HeraErrno DevicePlugin::adjust_parameter(const std::string& type, const std::string& value)
{
    (void)value;
    if (type == "EnableImu" && runtime_) {
        std::lock_guard<std::mutex> lock(runtime_->mutex);
        runtime_->enable_imu = local_parameters_.get_EnableImu();
    }
    return HeraErrno::OK;
}

#endif

data::SensorDataPtr DevicePlugin::do_convert(const data::DeviceDataPtr& storage_data,
                                             const ParametersInterface* parameters)
{
    (void)parameters;

    if (storage_data->is_type(LivoxImuPacket::TypeVal)) {
        auto raw_data = static_cast<LivoxImuPacket*>(storage_data.get());

        auto length = static_cast<uint32_t>(sizeof(data::ImuMagneticField));
        auto sensor_data = data::SensorData::create_from(storage_data, SensorDataType::ImuMagneticField, length);
        auto imu_data = static_cast<data::ImuMagneticField*>(sensor_data.get());

        imu_data->timestamp_intrinsic_ns = raw_data->timestamp_device_ns;
        imu_data->angular_velocity[0] = raw_data->gyro_x;
        imu_data->angular_velocity[1] = raw_data->gyro_y;
        imu_data->angular_velocity[2] = raw_data->gyro_z;
        imu_data->linear_acceleration[0] = raw_data->acc_x;
        imu_data->linear_acceleration[1] = raw_data->acc_y;
        imu_data->linear_acceleration[2] = raw_data->acc_z;
        imu_data->magnetic_field[0] = 0;
        imu_data->magnetic_field[1] = 0;
        imu_data->magnetic_field[2] = 0;
        return sensor_data;
    }

    return data::SensorData::broken_data();
}

}  // namespace livox
}  // namespace lidar
}  // namespace device
}  // namespace hera
}  // namespace wayz
