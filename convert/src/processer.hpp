///
/// @file processer.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Header of class Processor
/// @version 0.1
/// @date 2019-11-13
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

#include "common/logger/logger.hpp"
#include "common/utils/remapper.hpp"
#include "devices/src/device_factory.hpp"
#include "ros_message.hpp"

namespace wayz {
namespace hera {
namespace convert {

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
    static auto create(const std::string& type,
                       const std::string& vendor,
                       const std::string& name,
                       const std::string& folder,
                       const Remapper* remapper)
    {
        return ProcesserPtr(new Processer(type, vendor, name, folder, remapper));
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

private:
    ///
    /// @brief Construct a new Processer object
    ///
    /// @param type category type of device, e.g., imu
    /// @param vendor vendor type of device, e.g., velodyne
    /// @param name name of device, e.g., front
    /// @param folder root folder of one record data
    /// @param remapper remapper for topic name and frame id
    Processer(const std::string& type,
              const std::string& vendor,
              const std::string& name,
              const std::string& folder,
              const Remapper* remapper);


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
    /// @param in_message a pointer to ROSMessage to publish
    /// @note This operation uses semaphore invoke global bag access
    /// @note This operation move a ROSMessage to public member 'message',
    /// so handler can read from it.
    void publish(ROSMessagePtr&& in_message);

    ///
    /// @brief Cast a Timestamp to ros header
    ///
    /// @param ts Timestamp
    /// @return ros::Time
    ros::Time to_ros_time(Timestamp ts);

    ///
    /// @brief Remap a frame_id/topic_name to
    ///
    /// @param str name to remap
    /// @return std::string remapped name
    std::string remap(std::string&& str);

    ///
    /// @brief Process a SensorData
    ///
    /// @tparam T SensorDataType sensor data type
    /// @param data pointer to SensorData
    /// @see folder: processer_impls
    template<SensorDataType T>
    void process(SensorDataPtr& data);

    ///
    /// @brief Thread function
    ///
    /// For reading sensor data from device and publishing ros messages
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
    ROSMessagePtr message;

private:
    const Remapper* const remapper_;  ///< (weak) pointer remapper for topic name and frame id
    const std::string frame_id_;      ///< frame_id of this device
    const std::string topic_prefix_;  ///< prefix of topic name, with slash

    DevicePtr const device_;  ///< unique pointer to device
    std::thread thread_;      ///< thread of reading sensor data and processing
};

}  // namespace convert
}  // namespace hera
}  // namespace wayz