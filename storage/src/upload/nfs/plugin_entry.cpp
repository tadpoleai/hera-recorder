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
#include "libnfs.h"
//
#include "libnfs-raw.h"
//
#include "libnfs-raw-mount.h"
//
#include <fcntl.h>
#include <iostream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <thread>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "common/include/logger/logger.hpp"

namespace wayz {
namespace hera {
namespace storage {
namespace upload {
namespace nfs {

HERA_UPLOAD_PLUGIN_DEFINE_START

std::unique_ptr<std::thread> thread_run_;

int source_file_handler_{-1};

struct nfs_context* nfs_context_{nullptr};
struct nfsfh* nfs_file_handler_{nullptr};

HERA_UPLOAD_PLUGIN_DEFINE_END

HERA_UPLOAD_PLUGIN_EXPORT("nfs");

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

    if (config_.destination.find("*") != std::string::npos || config_.destination.find("?") != std::string::npos ||
        config_.destination.find(":") == std::string::npos) {
        set_error("Invalid remote schema, should be server:/path");
        return;
    }

    // Tokenize NFS Schema
    auto p = config_.destination.find(":");
    std::string nfs_hostname = config_.destination.substr(0, p);
    std::string nfs_mountpoint = config_.destination.substr(p + 1);

    log::debug << "NFS Hostname = " << nfs_hostname << log::endl;
    log::debug << "NFS Mountpoint = " << nfs_mountpoint << log::endl;

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

    /// Initialize nfs context
    nfs_context_ = nfs_init_context();
    if (!nfs_context_) {
        set_error("Can not initialize nfs context");
        return;
    }

    /// Mount
    if (nfs_mount(nfs_context_, nfs_hostname.c_str(), nfs_mountpoint.c_str()) != 0) {
        set_error(std::string("Can not mount nfs, ") + nfs_get_error(nfs_context_));
        return;
    }

    /// Create/open destination file (temp)
    constexpr auto write_flags = O_WRONLY | O_CREAT | O_TRUNC;
    std::string destfile_basename = config_.source.substr(config_.source.find_last_of("/\\") + 1);
    std::string destfile_basename_temp = destfile_basename + ".uploading";

    if (nfs_open(nfs_context_, destfile_basename_temp.c_str(), write_flags, &nfs_file_handler_) != 0) {
        set_error(std::string("Failed to create/open remote file to write, ") + nfs_get_error(nfs_context_));
        return;
    }

    thread_run_ = std::make_unique<std::thread>([=] {
        constexpr size_t BufSize = 4ULL << 20;  // 8MiB;
        auto buf = std::make_unique<char[]>(BufSize);
        size_t read_size = 0;
        size_t processed_size = 0;

        auto start_time = time::Timestamp::now();
        status_.stage = Stage::InProgress;

        do {
            read_size = read(source_file_handler_, buf.get(), BufSize);
            auto written_size = nfs_write(nfs_context_, nfs_file_handler_, read_size, buf.get());
            if (written_size < 0) {
                set_error(std::string("Failed to write on remote file, ") + nfs_get_error(nfs_context_));
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

        nfs_close(nfs_context_, nfs_file_handler_);

        if (status_.stage != Stage::Error) {
            if (nfs_rename(nfs_context_, destfile_basename_temp.c_str(), destfile_basename.c_str()) != 0) {
                set_error(std::string("Failed to rename temp file, ") + nfs_get_error(nfs_context_));
            } else {
                status_.stage = Stage::Finished;
            }
        }

        nfs_destroy_context(nfs_context_);
        nfs_context_ = nullptr;
    });
}

UploadPlugin::~UploadPlugin()
{
    if (thread_run_) {
        thread_run_->join();
    }

    if (nfs_context_) {
        nfs_destroy_context(nfs_context_);
    }

    log::debug << "~Nfs" << log::endl;
}

void UploadPlugin::terminate()
{
    return set_error("Terminated by user");
}

}  // namespace nfs
}  // namespace upload
}  // namespace storage
}  // namespace hera
}  // namespace wayz