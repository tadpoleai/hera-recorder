///
/// @file processer.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Processor
/// @version 0.1
/// @date 2019-12-19
///
/// @copyright Copyright (c) 2019
///

#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <semaphore.h>
#include <string>
#include <thread>

#include "devices/src/device_factory.hpp"

namespace wayz {
namespace hera {
namespace replay {

class Processer;

///
/// @brief An unique pointer to Processer
///
using ProcesserPtr = std::unique_ptr<Processer>;

///
/// @brief Process from SensorData to ROSMessage, controlled by semaphore
///
/// @see AlignedConverter
class Processer {
public:
    ///
    /// @brief Create a Processer
    ///
    /// @see Processer();
    static auto create(const uint32_t id,
                       const std::string& type,
                       const std::string& vendor,
                       const std::string& name,
                       const std::string& folder)
    {
        return ProcesserPtr(new Processer(id, type, vendor, name, folder));
    }

public:
    ///
    /// @brief Get processed storage data size
    ///
    /// @return uint64_t processed s`torage data size
    inline uint64_t processed_size() const noexcept
    {
        if (device_) {
            return device_->get_volume();
        } else {
            return 0;
        }
    }

    ///
    /// @brief Get if processer's device is valid and opened
    ///
    /// @return bool is_open is processer's device valid
    inline bool is_open() const noexcept
    {
        return (device_ != nullptr);
    }

    ///
    /// @brief Get the id of device
    ///
    /// @return uint32_t id of device
    inline uint32_t get_id() const noexcept
    {
        if (is_open()) {
            return device_->get_id();
        } else {
            return INT32_MAX;
        }
    }

private:
    ///
    /// @brief Construct a new Processer object
    ///
    /// @param id id of device
    /// @param type category type of device, e.g., imu
    /// @param vendor vendor type of device, e.g., velodyne
    /// @param name name of device, e.g., front
    /// @param folder root folder of one record data
    /// @param remapper remapper for topic name and frame id
    Processer(const uint32_t id,
              const std::string& type,
              const std::string& vendor,
              const std::string& name,
              const std::string& folder);

public:
    Processer(const Processer&) = delete;

    ///
    /// @brief Destroy the Processer object
    ///
    ~Processer();

private:
    ///
    /// @brief Publish a message
    ///
    /// @param in_message a pointer to SensorData to publish
    /// @note This operation uses semaphore invoke global bag access
    ///
    void publish(SensorDataPtr&& in_message);

    ///
    /// @brief Process a SensorData
    ///
    /// @param data pointer to SensorData
    /// @see folder: processer_impls
    void process(SensorDataPtr& data);

    ///
    /// @brief Thread function
    ///
    /// For reading sensor data from device and publishing messages
    void thread_function();

public:
    ///
    /// @brief Semaphore for publish / conversion
    ///
    /// Wait this semaphore before a new message can be published,
    sem_t* publish_sem;

    ///
    /// @brief Semaphore for bag receiving / bagfile writing
    ///
    /// Wait this semaphore before message can be read from member 'message'
    sem_t* receive_sem;

    ///
    /// @brief Published message
    ///
    /// @note Both R/W access to it should protected by semaphore
    SensorDataPtr message;

private:
    DevicePtr const device_;  ///< unique pointer to device
    std::thread thread_;      ///< thread of reading sensor data and processing
};

}  // namespace replayer
}  // namespace hera
}  // namespace wayz