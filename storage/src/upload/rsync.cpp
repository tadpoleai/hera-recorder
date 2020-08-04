///
/// @file rsync.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Wrapper of shell program rsync
/// @date 2020-05-13
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "rsync.hpp"

#include <algorithm>
#include <errno.h>
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

Rsync::Rsync(const Config& config)
{
    config_ = config;

    if (config_.source.empty() || config_.remote_destination.empty()) {
        set_error("Local or Remote is empty");
        return;
    }

    if (config_.source.find("*") != std::string::npos || config_.source.find("?") != std::string::npos) {
        set_error("Invalid local file name");
        return;
    }
    if (config_.remote_destination.find("*") != std::string::npos ||
        config_.remote_destination.find("?") != std::string::npos) {
        set_error("Invalid remote file name");
        return;
    }

    /// Check local file existance
    status_.total_size = file::get_file_size(config_.source);
    if (status_.total_size == 0) {
        set_error("Local file is empty or can not be read");
        return;
    }

    std::vector<std::string> argv = {
            "rsync",
            "--info=progress",
            "--archive",
            // "--checksum",
            "--partial",
            "--timeout=15",
    };
    if (config_.compress) {
        argv.emplace_back("--compress");
    }

    argv.emplace_back(config_.source);

    argv.emplace_back(config_.remote_destination);

    log::info << "Rsync: opening pipe" << log::endl;
    process_ = std::make_unique<common::Process>(argv);
    if (process_->get_pid() < 0 || !process_->get_stdout() || !process_->get_stderr()) {
        set_error("Can not open pipe");
        return;
    }
    log::info << "Rsync: invoked rsync command" << log::endl;

    thread_err_ = std::make_unique<std::thread>([=] {
        std::string buffer;
        buffer.reserve(64);
        while (true) {
            int c = ::fgetc(process_->get_stderr());
            if (c == EOF) {
                break;
            }

            if (c != '\n' && c != '\r') {
                buffer.push_back(c);
                continue;
            }

            if (buffer.empty()) {
                continue;
            }

            set_error(buffer);
            buffer.clear();
            buffer.reserve(64);
        }
    });

    thread_out_ = std::make_unique<std::thread>([=] {
        std::string buffer;
        buffer.reserve(64);
        while (true) {
            int c = ::fgetc(process_->get_stdout());
            if (c == EOF) {
                break;
            }

            if (c != '\n' && c != '\r') {
                buffer.push_back(c);
                continue;
            }

            if (buffer.empty()) {
                continue;
            }

            parse(buffer);
            buffer.clear();
            buffer.reserve(64);
        }

        thread_err_->join();
        log::info << "Rsync: closing pipe" << log::endl;
        process_.reset();

        switch (status_.stage) {
        case Stage::Inited:
            // pass-through
        case Stage::InProgress:
            // pass-through
        case Stage::Finished:
            status_.stage = Stage::Finished;
            log::info << "Rsync: finished" << log::endl;
            break;
        case Stage::Error:
            log::error << "Rsync: " << status_.error_reason << log::endl;
            break;
        }
    });
}  // namespace upload

Rsync::~Rsync()
{
    if (thread_out_) {
        thread_out_->join();
    }
    log::debug << "~Rsync" << log::endl;
}

void Rsync::terminate()
{
    if (process_->get_pid() > 0) {
        log::info << "Rsync: Killing rsync, pid = " << process_->get_pid() << log::endl;
        process_->terminate();
    }

    return set_error("Terminated by user");
}

void Rsync::parse(const std::string& line)
{
    if (status_.stage == Stage::Inited) {
        status_.stage = Stage::InProgress;
    }

    try {
        std::vector<std::string> tokens;

        std::string token;
        std::istringstream liness(line);
        while (tokens.size() < 4 && std::getline(liness, token, ' ')) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }

        if (tokens.size() == 4) {
            tokens[0].erase(std::remove(tokens[0].begin(), tokens[0].end(), ','), tokens[0].end());
            status_.processed_size = stoull(tokens[0]);

            status_.speed_literal = tokens[2];

            status_.eta = tokens[3];
        }
    } catch (std::exception& e) {
        log::debug << e.what() << log::endl;
    }
}

}  // namespace upload
}  // namespace storage
}  // namespace hera
}  // namespace wayz