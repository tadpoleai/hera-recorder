///
/// @file plugin_entry.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2021-03-05
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "../plugin_base.hpp"
//

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <errno.h>
#include <iostream>
#include <paths.h>
#include <regex>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/wait.h>

#include "common/include/logger/logger.hpp"
#include "common/include/utils/folder_content.hpp"

namespace wayz {
namespace hera {
namespace storage {
namespace upload {
namespace local {

HERA_UPLOAD_PLUGIN_DEFINE_START

std::unique_ptr<std::thread> thread_;

HERA_UPLOAD_PLUGIN_DEFINE_END

HERA_UPLOAD_PLUGIN_EXPORT("local");

UploadPlugin::UploadPlugin(const Config& config)
{
    config_ = config;

    if (config_.source.empty() || config_.destination.empty()) {
        set_error("Local or Remote is empty");
        return;
    }

    if (config_.source.find("*") != std::string::npos || config_.source.find("?") != std::string::npos) {
        set_error("Invalid local file name");
        return;
    }

    if (config_.destination.find("*") != std::string::npos || config_.destination.find("?") != std::string::npos) {
        set_error("Invalid remote file name");
        return;
    }

    /// Check local file existance
    status_.total_size = file::get_file_size(config_.source);
    if (status_.total_size == 0) {
        set_error("Local file is empty or can not be read");
        return;
    }

    /// Check destination fs space
    auto space = file::get_filesystem_status(config.destination).free_space;
    if (space < status_.total_size) {
        set_error("Destination file system has no enough space");
    }

    std::string dest_file = config_.destination;
    auto pos = config_.source.rfind('/');
    if (pos == std::string::npos) {
        dest_file += config_.source;
    } else {
        dest_file += config_.source.substr(pos);
    }

    thread_ = std::make_unique<std::thread>([=] {
        FILE* source = fopen(config_.source.c_str(), "rb");
        if (!source) {
            return set_error("Can not open source file " + config_.source);
        }
        FILE* dest = fopen(dest_file.c_str(), "wb");
        if (!dest) {
            return set_error("Can not open destination file " + dest_file);
        }

        auto start_time = time::Timestamp::now();
        status_.stage = Stage::InProgress;

        size_t read_size = 0;
        size_t processed_size = 0;
        constexpr size_t BufSize = 4ULL << 20;  // 8MiB;
        auto buf = std::make_unique<char[]>(BufSize);
        do {
            try {
                read_size = fread(buf.get(), 1, BufSize, source);
                auto written_size = fwrite(buf.get(), 1, read_size, dest);
                if (read_size != (size_t)written_size) {
                    set_error("Failed to write file, size mismatched");
                }
            } catch (std::exception& e) {
                set_error(std::string("Caught error, ") + e.what());
            }

            processed_size += read_size;

            auto current_time = time::Timestamp::now();
            auto escaped_duration = current_time - start_time;
            double escaped_duration_sec = escaped_duration / (double)time::OneSecond;
            time::Duration eta = (status_.total_size / (double)processed_size - 1) * escaped_duration;
            status_.eta = eta.to_str_second();

            file::FileSize overall_speed = processed_size / escaped_duration_sec;
            std::stringstream speed_literal;
            speed_literal << overall_speed << "/s";
            status_.speed_literal = speed_literal.str();

            status_.processed_size = processed_size;

        } while (read_size != 0 && status_.stage != Stage::Error);

        fclose(source);
        fclose(dest);

        if (status_.stage != Stage::Error) {
            status_.stage = Stage::Finished;
        }
    });
}

UploadPlugin::~UploadPlugin()
{
    if (thread_) {
        thread_->join();
    }
    log::debug << "~Local" << log::endl;
}

void UploadPlugin::terminate()
{
    return set_error("Terminated by user");
}

}  // namespace local
}  // namespace upload
}  // namespace storage
}  // namespace hera
}  // namespace wayz
