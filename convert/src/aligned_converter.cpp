///
/// @file aligned_converter.cpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Implementation of class AlignedConverter
/// @version 0.1
/// @date 2019-11-13
///
/// @copyright Copyright (c) 2019
///

#include "aligned_converter.hpp"

#include "common/utils/get_folder_content.hpp"

namespace wayz {
namespace hera {
namespace convert {

/// Scan the recorded data folder and construct processers.
///
/// Open the bag file for writing, and init the writing thread
AlignedConverter::AlignedConverter(const std::string& folder,
                                   const std::string& bagfile,
                                   RemapperPtr&& remapper) :
    total_size_(0),
    bag_(bagfile, false),
    remapper_(std::move(remapper))
{
    running_ = true;
    auto types = get_folder_content(folder);
    if (!types.opened) {
        return;
    }
    for (const auto& type : types.folders) {
        auto vendors = get_folder_content(type.fullname);
        if (!vendors.opened) {
            continue;
        }

        for (const auto& vendor : vendors.folders) {
            auto devices = get_folder_content(vendor.fullname);
            if (!devices.opened) {
                continue;
            }

            for (const auto& device : devices.folders) {
                processers_.emplace_back(Processer::create(
                        type.basename, vendor.basename, device.basename, folder, remapper_.get()));

                auto device_size = get_folder_content(device.fullname).total_size;
                total_size_ = total_size_ + device_size;
                log::debug << "Add device " << type.basename << "/" << vendor.basename << "/"
                           << device.basename << ", sized " << device_size << log::endl;
            }
        }
    }

    write_thread_ = std::thread(&AlignedConverter::write_thread_function, this);
}

/// Wait for thread to join, and close bag file
///
AlignedConverter::~AlignedConverter()
{
    write_thread_.join();
    log::info << "Flushing to file system" << log::endl;
    log::info << "This may take a while depending on destination file system" << log::endl;
    bag_.close();
    log::info << "Flushed to file system" << log::endl;
}


FileSize AlignedConverter::converted_size() const
{
    uint64_t result = 0;
    for (const auto& processer : processers_) {
        result += processer->processed_size();
    }
    return result;
}

FileSize AlignedConverter::total_size() const noexcept
{
    return total_size_;
}


bool AlignedConverter::running() const noexcept
{
    return running_;
}

void AlignedConverter::write_thread_function()
{
    Processer* earliest_processer = nullptr;

    /// In a loop.
    while (true) {
        if (earliest_processer == nullptr) {
            /// Firstly,
            /// wait for all processer for publishing their first message.
            for (auto& processer : processers_) {
                sem_wait(processer->receive_sem);
            }
        } else {
            /// Or, wait for last processer for a new message.
            /// Here, the last processer is the processer that published a message
            /// with earliest timestamp in last round.
            sem_wait(earliest_processer->receive_sem);
        }

        int64_t earliest_timestamp = LONG_MAX;
        earliest_processer = nullptr;
        /// Search among all processers to determine earliest message,
        /// and get its handler, until all processers published EndOfFile.
        for (auto& processer : processers_) {
            if (processer->message->type != ROSMessageType::EndOfFile) {
                if (earliest_timestamp > processer->message->timestamp_ns) {
                    earliest_timestamp = processer->message->timestamp_ns;
                    earliest_processer = processer.get();
                }
            }
        }

        /// If all processers published EndOfFile, break the loop
        if (!earliest_processer) {
            break;
        }

        /// Write the message into bag file
        bag_ << std::move(earliest_processer->message);

        /// Notify the last processer the publish next message.
        sem_post(earliest_processer->publish_sem);
    }
    running_ = false;
}

}  // namespace convert
}  // namespace hera
}  // namespace wayz