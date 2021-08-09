///
/// @file plugin_entry.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief
/// @date 2021-03-05
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include <libsmbclient.h>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>

#include "../plugin_base.hpp"
#include "common/include/logger/logger.hpp"

namespace wayz {
namespace hera {
namespace storage {
namespace upload {
namespace smb {

HERA_UPLOAD_PLUGIN_DEFINE_START

std::unique_ptr<std::thread> thread_run_;

int source_file_handler_{-1};

HERA_UPLOAD_PLUGIN_DEFINE_END

HERA_UPLOAD_PLUGIN_EXPORT("smb");

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

    /// Briefly check remote destination schema
    if (config_.destination.find("*") != std::string::npos || config_.destination.find("?") != std::string::npos ||
        config_.destination.find(":") == std::string::npos || config_.destination.find("@") == std::string::npos ||
        config_.destination.find("smb://") == std::string::npos) {
        set_error("Invalid remote destination schema, should be "
                  "'smb://[<workgroup>;]<username>:<password>@<address>/<share>/<path>'");
        return;
    }

    /// Check local file existance
    status_.total_size = hera::file::get_file_size(config_.source);
    if (status_.total_size == 0) {
        set_error("Local file is empty or can not be read");
        return;
    }

    /// Open source file to read
    source_file_handler_ = ::open(config_.source.c_str(), O_RDONLY);
    if (source_file_handler_ < 0) {
        set_error("Local file can not be open to read");
        return;
    }

    const auto destfile_url = config_.destination + "/" + config_.source.substr(config_.source.find_last_of("/\\") + 1);
    const auto destfile_url_temp = destfile_url + ".uploading";

    thread_run_ = std::make_unique<std::thread>([=] {
        std::unique_lock<std::mutex> lock(upload_mutex_);

        constexpr size_t BufSize = 4ULL << 20;  // 8MiB;
        auto buf = std::make_unique<char[]>(BufSize);
        size_t read_size = 0;
        size_t processed_size = 0;

        SMBCCTX* smb_ctx = smbc_new_context();
        if (!smb_ctx) {
            return set_error("smbc_new_context failed");
        }
        if (!smbc_init_context(smb_ctx)) {
            return set_error("smbc_init_context failed");
        }
        smbc_set_context(smb_ctx);
        if (smbc_init(nullptr, 0) < 0) {
            return set_error("smbc_init failed");
        }
        int remote_fd = smbc_creat(destfile_url_temp.c_str(), 0644);
        if (remote_fd < 0) {
            return set_error("smbc_creat failed");
        }

        auto start_time = time::Timestamp::now();
        status_.stage = Stage::InProgress;

        do {
            read_size = ::read(source_file_handler_, buf.get(), BufSize);
            auto written_size = smbc_write(remote_fd, buf.get(), read_size);
            if (written_size < 0) {
                set_error(std::string("Failed to write on remote file"));
            } else if (read_size != (size_t)written_size) {
                set_error("Failed to write on remote file, size mismatched");
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

        ::close(source_file_handler_);
        smbc_close(remote_fd);

        if (status_.stage != Stage::Error) {
            auto err = smbc_rename(destfile_url_temp.c_str(), destfile_url.c_str());
            if (err < 0) {
                set_error(std::string("Failed to rename temp file"));
            } else {
                status_.stage = Stage::Finished;
            }
        };

        smbc_free_context(smb_ctx, 1);

        lock.unlock();
    });
}

UploadPlugin::~UploadPlugin()
{
    if (thread_run_) {
        thread_run_->join();
    }

    log::debug << "~Smb" << log::endl;
}

void UploadPlugin::terminate()
{
    return set_error("Terminated by user");
}

}  // namespace smb
}  // namespace upload
}  // namespace storage
}  // namespace hera
}  // namespace wayz