///
/// @file extract_insta.cpp
/// @brief Extract Insta360 video and gyro data from .hera storage
///
/// Usage: ./hera-extract-insta <input.hera> [--video <file.h264>] [--gyro <file.csv>]
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

// Packet type codes
constexpr device::DeviceDataType kInstaVideoPacketType = 0x0421;
constexpr device::DeviceDataType kInstaGyroPacketType = 0x0422;

// Packet header offsets within payload
#pragma pack(push, 1)

struct VideoPacketHeader {
    int64_t timestamp_device_ns;
    uint64_t timestamp_host_ns;
    uint8_t stream_type;
    int32_t stream_index;
    uint32_t payload_size;
};

struct GyroPacketHeader {
    uint64_t timestamp_host_ns;
    uint32_t sample_count;
    uint32_t payload_size;
};

// Matches ins_camera::GyroData from the Insta360 SDK (stream_types.h):
//   timestamp: int64_t (NOT int32, NOT milliseconds)
//   6 sensor values: double (NOT float)
//   field order: timestamp, ax, ay, az, gx, gy, gz
struct GyroSample {
    int64_t timestamp;    ///< Timestamp in nanoseconds or raw units from SDK
    double ax;
    double ay;
    double az;
    double gx;
    double gy;
    double gz;
};

#pragma pack(pop)

bool extract_video(const std::string& storage_file, const std::string& output_file) {
    if (output_file.empty()) {
        return true;
    }

    auto storage = storage::StorageManager::open(storage_file, true);
    if (!storage) {
        std::cerr << "Failed to open storage: " << storage_file << std::endl;
        return false;
    }

    std::ofstream out(output_file, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open output file: " << output_file << std::endl;
        return false;
    }

    int64_t video_packets = 0;
    int64_t video_bytes = 0;

    std::cout << "Extracting video frames..." << std::endl;

    while (auto data = storage->read()) {
        if (data && data->is_type(kInstaVideoPacketType)) {
            try {
                // Cast shared_ptr to the actual implementation type
                // The payload is stored immediately after the DeviceData header
                const uint8_t* packet_ptr = reinterpret_cast<const uint8_t*>(data.get());
                
                // Offset to payload: after DeviceData header (uint32_t + uint32_t + uint16_t + uint16_t + uint32_t + uint64_t = 24 bytes)
                constexpr size_t HEADER_SIZE = 24;
                const uint8_t* payload = packet_ptr + HEADER_SIZE;
                
                // Ensure we have enough data for the video header
                if (data->get_length() > HEADER_SIZE + sizeof(VideoPacketHeader)) {
                    const VideoPacketHeader* vid_header = reinterpret_cast<const VideoPacketHeader*>(payload);
                    
                    // Extract H.264/H.265 bitstream
                    const uint8_t* video_data = payload + sizeof(VideoPacketHeader);
                    uint32_t video_size = vid_header->payload_size;
                    
                    if (video_size > 0 && video_size < data->get_length()) {
                        if (out.write(reinterpret_cast<const char*>(video_data), video_size)) {
                            video_packets++;
                            video_bytes += video_size;
                        }
                    }
                }
            } catch (...) {
                // Skip malformed packets
                continue;
            }
        }
    }

    out.close();
    storage.reset();

    std::cout << "✓ Video extraction complete: " << video_packets << " frames, " 
              << std::fixed << std::setprecision(2) 
              << (video_bytes / (1024.0 * 1024.0)) << " MiB" << std::endl;

    return video_packets > 0;
}

bool extract_gyro(const std::string& storage_file, const std::string& output_file) {
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

    // Write CSV header (matches ins_camera::GyroData in stream_types.h)
    out << "timestamp_ns,ax,ay,az,gx,gy,gz" << std::endl;

    int64_t gyro_packets = 0;
    int64_t gyro_samples = 0;

    std::cout << "Extracting gyro data..." << std::endl;

    while (auto data = storage->read()) {
        if (data && data->is_type(kInstaGyroPacketType)) {
            try {
                const uint8_t* packet_ptr = reinterpret_cast<const uint8_t*>(data.get());
                constexpr size_t HEADER_SIZE = 24;
                const uint8_t* payload = packet_ptr + HEADER_SIZE;
                
                if (data->get_length() > HEADER_SIZE + sizeof(GyroPacketHeader)) {
                    const GyroPacketHeader* gyro_header = reinterpret_cast<const GyroPacketHeader*>(payload);
                    
                    // Parse gyro samples
                    const GyroSample* samples = reinterpret_cast<const GyroSample*>(
                        payload + sizeof(GyroPacketHeader)
                    );

                    const uint32_t sample_bytes = gyro_header->payload_size;
                    const uint32_t max_samples = sample_bytes / static_cast<uint32_t>(sizeof(GyroSample));
                    const uint32_t sample_count = std::min(gyro_header->sample_count, max_samples);

                    for (uint32_t i = 0; i < sample_count; ++i) {
                        out << samples[i].timestamp << ","
                            << std::fixed << std::setprecision(6)
                            << samples[i].ax << ","
                            << samples[i].ay << ","
                            << samples[i].az << ","
                            << samples[i].gx << ","
                            << samples[i].gy << ","
                            << samples[i].gz << std::endl;
                        
                        gyro_samples++;
                    }
                    
                    gyro_packets++;
                }
            } catch (...) {
                continue;
            }
        }
    }

    out.close();
    storage.reset();

    std::cout << "✓ Gyro extraction complete: " << gyro_packets << " packets, " 
              << gyro_samples << " samples" << std::endl;

    return gyro_packets > 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.hera> [--video <file.h264>] [--gyro <file.csv>]"
                  << std::endl;
        return 1;
    }

    std::string storage_file = argv[1];
    std::string video_out;
    std::string gyro_out;

    // Parse arguments
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--video" && i + 1 < argc) {
            video_out = argv[++i];
        } else if (arg == "--gyro" && i + 1 < argc) {
            gyro_out = argv[++i];
        }
    }

    if (video_out.empty() && gyro_out.empty()) {
        std::cerr << "No output files specified. Use --video and/or --gyro" << std::endl;
        return 1;
    }

    bool success = true;

    if (!video_out.empty()) {
        success = extract_video(storage_file, video_out) && success;
    }

    if (!gyro_out.empty()) {
        success = extract_gyro(storage_file, gyro_out) && success;
    }

    return success ? 0 : 1;
}