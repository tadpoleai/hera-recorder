///
/// @file extract_mid360.cpp
/// @brief Extract Livox Mid360 point cloud and IMU data from .hera storage
///
/// Usage:
///   ./hera-extract-mid360 <input.hera> [--points <file.csv>] [--imu <file.csv>]
///
/// Point cloud CSV columns:
///   timestamp_device_ns, timestamp_host_ns, x_m, y_m, z_m, reflectivity, tag, data_type
///
/// IMU CSV columns:
///   timestamp_device_ns, timestamp_host_ns, gyro_x, gyro_y, gyro_z, acc_x, acc_y, acc_z
///

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#ifdef HERA_COMPILE_IN_REPO
#include "storage/include/storage.hpp"
#include "device/include/device_data.hpp"
#else
#include <hera/storage/storage.hpp>
#include <hera/device/device_data.hpp>
#endif

using namespace wayz::hera;

// Packet type codes for lidar/livox (from plugin_data.hpp)
constexpr device::DeviceDataType kLivoxPointPacketType    = 0x0521;  // LivoxPacket (full-synced)
constexpr device::DeviceDataType kLivoxPointPacketUnSynced = 0x0522;
constexpr device::DeviceDataType kLivoxPointPacketLocalSynced = 0x0523;
constexpr device::DeviceDataType kLivoxImuPacketType      = 0x0524;  // LivoxImuPacket

// Livox raw payload data types
constexpr uint8_t kCartesianHighData = 0x01;
constexpr uint8_t kCartesianLowData  = 0x02;

// DeviceData header is 24 bytes:
//   uint32_t length (4) + uint32_t device_id (4) +
//   uint16_t vendor_type (2) + uint16_t msg_type (2) +
//   uint32_t sequence (4) + uint64_t timestamp_receive_ns (8)
constexpr size_t kDeviceDataHeaderSize = 24;

#pragma pack(push, 1)

/// Matches LivoxPacket fields (from plugin_data.hpp) after DeviceData header
struct LivoxPointPacketHeader {
    uint64_t timestamp_device_ns;
    uint64_t timestamp_host_ns;
    uint32_t handle;
    uint8_t  livox_dev_type;
    uint8_t  livox_data_type;
    uint8_t  livox_time_type;
    uint8_t  frame_cnt;
    uint16_t dot_num;
    uint16_t packet_length;
    uint32_t payload_size;
    // uint8_t payload[0] follows
};

/// Matches LivoxImuPacket fields (from plugin_data.hpp) after DeviceData header
struct LivoxImuPacketHeader {
    uint64_t timestamp_device_ns;
    uint64_t timestamp_host_ns;
    uint32_t handle;
    uint8_t  livox_dev_type;
    uint8_t  livox_time_type;
    float    gyro_x;
    float    gyro_y;
    float    gyro_z;
    float    acc_x;
    float    acc_y;
    float    acc_z;
    uint32_t payload_size;
    // uint8_t payload[0] follows
};

/// High precision (1mm) Cartesian point — payload data type 0x01
struct LivoxCartesianHighPoint {
    int32_t x_mm;
    int32_t y_mm;
    int32_t z_mm;
    uint8_t reflectivity;
    uint8_t tag;
};

/// Low precision (1cm) Cartesian point — payload data type 0x02
struct LivoxCartesianLowPoint {
    int16_t x_cm;
    int16_t y_cm;
    int16_t z_cm;
    uint8_t reflectivity;
    uint8_t tag;
};

#pragma pack(pop)

bool extract_points(const std::string& storage_file, const std::string& output_file)
{
    if (output_file.empty()) {
        return true;
    }

    auto storage = storage::StorageManager::open(storage_file, true);
    if (!storage) {
        std::cerr << "Failed to open storage: " << storage_file << std::endl;
        return false;
    }

    std::ofstream out(output_file);
    if (!out) {
        std::cerr << "Failed to open output file: " << output_file << std::endl;
        return false;
    }

    out << "timestamp_device_ns,timestamp_host_ns,x_m,y_m,z_m,reflectivity,tag,data_type\n";

    int64_t point_packets = 0;
    int64_t total_points  = 0;

    std::cout << "Extracting point cloud data..." << std::endl;

    while (auto data = storage->read()) {
        if (!data) continue;
        bool is_point = data->is_type(kLivoxPointPacketType) ||
                        data->is_type(kLivoxPointPacketUnSynced) ||
                        data->is_type(kLivoxPointPacketLocalSynced);
        if (!is_point) continue;

        try {
            const uint8_t* base    = reinterpret_cast<const uint8_t*>(data.get());
            const uint8_t* payload_ptr = base + kDeviceDataHeaderSize;

            if (data->get_length() <= kDeviceDataHeaderSize + sizeof(LivoxPointPacketHeader)) {
                continue;
            }

            const auto* hdr = reinterpret_cast<const LivoxPointPacketHeader*>(payload_ptr);
            const uint8_t* pts_ptr = payload_ptr + sizeof(LivoxPointPacketHeader);
            const uint64_t ts_dev  = hdr->timestamp_device_ns;
            const uint64_t ts_host = hdr->timestamp_host_ns;
            const uint8_t  dtype   = hdr->livox_data_type;

            if (dtype == kCartesianHighData) {
                const auto* pts = reinterpret_cast<const LivoxCartesianHighPoint*>(pts_ptr);
                const uint32_t count = hdr->payload_size / sizeof(LivoxCartesianHighPoint);
                for (uint32_t i = 0; i < count; ++i) {
                    out << ts_dev  << ","
                        << ts_host << ","
                        << std::fixed << std::setprecision(4)
                        << (pts[i].x_mm * 0.001) << ","
                        << (pts[i].y_mm * 0.001) << ","
                        << (pts[i].z_mm * 0.001) << ","
                        << static_cast<int>(pts[i].reflectivity) << ","
                        << static_cast<int>(pts[i].tag) << ","
                        << static_cast<int>(dtype) << "\n";
                }
                total_points += count;
            } else if (dtype == kCartesianLowData) {
                const auto* pts = reinterpret_cast<const LivoxCartesianLowPoint*>(pts_ptr);
                const uint32_t count = hdr->payload_size / sizeof(LivoxCartesianLowPoint);
                for (uint32_t i = 0; i < count; ++i) {
                    out << ts_dev  << ","
                        << ts_host << ","
                        << std::fixed << std::setprecision(3)
                        << (pts[i].x_cm * 0.01) << ","
                        << (pts[i].y_cm * 0.01) << ","
                        << (pts[i].z_cm * 0.01) << ","
                        << static_cast<int>(pts[i].reflectivity) << ","
                        << static_cast<int>(pts[i].tag) << ","
                        << static_cast<int>(dtype) << "\n";
                }
                total_points += count;
            }
            ++point_packets;
        } catch (...) {
            // Skip malformed packets
        }
    }

    std::cout << "✓ Point cloud extraction complete: "
              << point_packets << " packets, " << total_points << " points\n";
    return true;
}

bool extract_imu(const std::string& storage_file, const std::string& output_file)
{
    if (output_file.empty()) {
        return true;
    }

    auto storage = storage::StorageManager::open(storage_file, true);
    if (!storage) {
        std::cerr << "Failed to open storage: " << storage_file << std::endl;
        return false;
    }

    std::ofstream out(output_file);
    if (!out) {
        std::cerr << "Failed to open output file: " << output_file << std::endl;
        return false;
    }

    out << "timestamp_device_ns,timestamp_host_ns,gyro_x,gyro_y,gyro_z,acc_x,acc_y,acc_z\n";

    int64_t imu_packets = 0;

    std::cout << "Extracting IMU data..." << std::endl;

    while (auto data = storage->read()) {
        if (!data || !data->is_type(kLivoxImuPacketType)) continue;

        try {
            const uint8_t* base = reinterpret_cast<const uint8_t*>(data.get());
            const uint8_t* payload_ptr = base + kDeviceDataHeaderSize;

            if (data->get_length() <= kDeviceDataHeaderSize + sizeof(LivoxImuPacketHeader)) {
                continue;
            }

            const auto* hdr = reinterpret_cast<const LivoxImuPacketHeader*>(payload_ptr);

            out << hdr->timestamp_device_ns << ","
                << hdr->timestamp_host_ns   << ","
                << std::fixed << std::setprecision(6)
                << hdr->gyro_x << ","
                << hdr->gyro_y << ","
                << hdr->gyro_z << ","
                << hdr->acc_x  << ","
                << hdr->acc_y  << ","
                << hdr->acc_z  << "\n";

            ++imu_packets;
        } catch (...) {
            // Skip malformed packets
        }
    }

    std::cout << "✓ IMU extraction complete: " << imu_packets << " packets\n";
    return true;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " <input.hera> [--points <file.csv>] [--imu <file.csv>]\n";
        return 1;
    }

    const std::string storage_file = argv[1];
    std::string points_file;
    std::string imu_file;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--points" && i + 1 < argc) {
            points_file = argv[++i];
        } else if (arg == "--imu" && i + 1 < argc) {
            imu_file = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    if (points_file.empty() && imu_file.empty()) {
        std::cerr << "Specify at least one output: --points <file.csv> or --imu <file.csv>\n";
        return 1;
    }

    bool ok = extract_points(storage_file, points_file);
    ok = extract_imu(storage_file, imu_file) && ok;

    return ok ? 0 : 1;
}
