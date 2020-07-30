///
/// @file frequecy_calculator.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Calculate realtime frequency of device
/// @date 2020-05-27
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

#include "common/include/utils/time.hpp"
#include "device/include/device.hpp"

namespace wayz {
namespace hera {
namespace daemon {

class FrequecyCalculator final {
public:
    ///
    /// @brief Construct a new Frequecy Calculator object
    ///
    /// @param devices_ptr Bare pointer to vector of devices
    /// @note Must destroy FrequecyCalculator instance before vector of devices resets,
    /// or segmentation fault happens
    ///
    FrequecyCalculator(std::vector<device::DevicePtr>* devices_ptr);

    FrequecyCalculator(const FrequecyCalculator&) = delete;
    FrequecyCalculator& operator=(const FrequecyCalculator&) = delete;

    ///
    /// @brief Destroy the Frequecy Calculator object
    ///
    ///
    ~FrequecyCalculator();

    inline float get_result(size_t index) const noexcept
    {
        if (index >= result_.size()) {
            return 0.0f;
        } else {
            return std::max(0.0f, result_[index]);
        }
    }

private:
    void thread_function();  ///< Thread function of frequecy calculation

private:
    std::vector<float> result_;                    ///< Result of calculation, i.e., smoothen frequencies
    std::vector<device::DevicePtr>* devices_ptr_;  ///< Bare pointer to vector of devices
    std::atomic<bool> running_;                    ///< Running flag to control thread
    std::thread* thread_;                          ///< Thread of frequecy calculation
};

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
