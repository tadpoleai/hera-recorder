///
/// @file process.cpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Handler of process and pipe
/// @date 2020-05-13
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#include "common/include/utils/process.hpp"

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

namespace wayz {
namespace hera {
namespace common {

Process::Process(const std::vector<std::string>& argv) : pid_(-1), out_(nullptr), err_(nullptr)
{
    if (argv.empty()) {
        return;
    }

    int pd_out[2], pd_err[2], pd_in[2];
    if (::pipe(pd_out) < 0) {
        return;
    }
    if (::pipe(pd_err) < 0) {
        return;
    }
    if (::pipe(pd_in) < 0) {
        return;
    }

    pid_ = ::fork();

    switch (pid_) {
    case -1:  // Failed
        return;
    case 0:  // Child
    {
        char** argv_ptr = (char**)::malloc(sizeof(char*) * (argv.size() + 1));
        for (size_t i = 0; i < argv.size(); ++i) {
            argv_ptr[i] = (char*)argv[i].c_str();
        }
        argv_ptr[argv.size()] = nullptr;

        (void)::close(pd_out[0]);
        if (pd_out[1] != STDOUT_FILENO) {
            (void)::dup2(pd_out[1], STDOUT_FILENO);
            (void)::close(pd_out[1]);
        }

        (void)::close(pd_err[0]);
        if (pd_err[1] != STDERR_FILENO) {
            (void)::dup2(pd_err[1], STDERR_FILENO);
            (void)::close(pd_err[1]);
        }

        (void)::close(pd_in[1]);
        if (pd_err[0] != STDIN_FILENO) {
            (void)::dup2(pd_err[0], STDIN_FILENO);
            (void)::close(pd_err[0]);
        }

        ::execvp(*argv_ptr, argv_ptr);
        ::_exit(127);
    }
    }

    // Parent
    out_ = fdopen(pd_out[0], "r");
    (void)::close(pd_out[1]);

    err_ = fdopen(pd_err[0], "r");
    (void)::close(pd_err[1]);

    in_ = fdopen(pd_in[1], "w");
    (void)::close(pd_in[0]);
}

Process::~Process()
{
    if (pid_ == -1) {
        return;
    }

    if (out_) {
        (void)::fclose(out_);
    }

    if (err_) {
        (void)::fclose(err_);
    }

    if (in_) {
        (void)::fclose(in_);
    }
}

void Process::terminate()
{
    if (pid_ > 0) {
        ::kill(pid_, SIGTERM);
    }
}

void Process::kill()
{
    if (pid_ > 0) {
        ::kill(pid_, SIGKILL);
    }
}

}  // namespace common
}  // namespace hera
}  // namespace wayz