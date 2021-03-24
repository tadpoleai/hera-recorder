//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <future>
#include <numeric>
#include <regex>
#include <thread>
#include <vector>

#include "common/include/utils/folder_content.hpp"
#include "common/include/utils/time.hpp"
#include "service.hpp"
#include "storage/include/storage.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::latestLogs(std::vector<LogMessage>& result)
{
    static constexpr auto MaxLogNum = 256;
    const int32_t start_index = log_messages_.size() - 1;
    int32_t end_index = 0;
    if (start_index > MaxLogNum) {
        end_index = start_index - MaxLogNum;
    }

    int32_t front_index = 0;
    for (int32_t back_index = start_index; back_index > end_index; --back_index) {
        LogMessage front_msg;
        const auto& LogMsg = log_messages_[back_index];
        front_msg.index = front_index++;
        front_msg.level = static_cast<int>(LogMsg.level);
        front_msg.tsSec = LogMsg.ts.tv_sec;
        front_msg.tsNsec = LogMsg.ts.tv_nsec;
        front_msg.message = LogMsg.str;
        front_msg.level = static_cast<int>(LogMsg.level);
        result.emplace_back(std::move(front_msg));
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz