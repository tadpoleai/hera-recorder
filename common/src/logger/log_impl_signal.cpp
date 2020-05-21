//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <regex>

#include "logger/logger.hpp"
#include "version.hpp"

namespace wayz {
namespace hera {
namespace log {
namespace impl {

std::set<int> Logger::signal_to_ignore_user_;

void Logger::ignore_signal(int signo)
{
    signal_to_ignore_user_.insert(signo);
}

void Logger::register_back_trace()
{
    static const std::set<int> SignalsToIgnore = {SIGCONT, SIGURG, SIGIO, SIGPOLL, SIGCHLD, SIGWINCH};
    for (auto i = 0; i <= SIGRTMAX; i++) {
        if (SignalsToIgnore.count(i) == 0) {
            ::signal(i, &Logger::singal_handler);
        }
    }
}

void Logger::singal_handler(int signo)
{
    switch (signo) {
    case SIGINT:
        log::error << "FATAL: Received SIGINT: Interactive attention signal." << log::endl;
        break;
    case SIGILL:
        log::error << "FATAL: Received SIGILL: Illegal instruction." << log::endl;
        break;
    case SIGABRT:
        log::error << "FATAL: Received SIGABRT: Abnormal termination." << log::endl;
        break;
    case SIGFPE:
        log::error << "FATAL: Received SIGFPE: Erroneous arithmetic operation." << log::endl;
        break;
    case SIGSEGV:
        log::error << "FATAL: Received SIGSEGV: Invalid access to storage." << log::endl;
        break;
    case SIGTERM:
        log::error << "FATAL: Received SIGTERM: Termination request." << log::endl;
        break;
    case SIGHUP:
        log::error << "FATAL: Received SIGHUP: Hangup." << log::endl;
        break;
    case SIGQUIT:
        log::error << "FATAL: Received SIGQUIT: Quit." << log::endl;
        break;
    case SIGTRAP:
        log::error << "FATAL: Received SIGTRAP: Trace/breakpoint trap." << log::endl;
        break;
    case SIGKILL:
        log::error << "FATAL: Received SIGKILL: Killed." << log::endl;
        break;
    case SIGBUS:
        log::error << "FATAL: Received SIGBUS: Bus error." << log::endl;
        break;
    case SIGSYS:
        log::error << "FATAL: Received SIGSYS: Bad system call." << log::endl;
        break;
    case SIGPIPE:
        log::error << "FATAL: Received SIGPIPE: Broken pipe." << log::endl;
        break;
    case SIGALRM:
        log::error << "FATAL: Received SIGALRM: Alarm clock." << log::endl;
        break;
    case SIGURG:
        log::error << "FATAL: Received SIGURG: Urgent data is available at a socket." << log::endl;
        break;
    case SIGSTOP:
        log::error << "FATAL: Received SIGSTOP: Stop, unblockable." << log::endl;
        break;
    case SIGTSTP:
        log::error << "FATAL: Received SIGTSTP: Keyboard stop." << log::endl;
        break;
    case SIGCONT:
        log::error << "FATAL: Received SIGCONT: Continue." << log::endl;
        break;
    case SIGCHLD:
        log::error << "FATAL: Received SIGCHLD: Child terminated or stopped." << log::endl;
        break;
    case SIGTTIN:
        log::error << "FATAL: Received SIGTTIN: Background read from control terminal." << log::endl;
        break;
    case SIGTTOU:
        log::error << "FATAL: Received SIGTTOU: Background write to control terminal." << log::endl;
        break;
    case SIGPOLL:
        log::error << "FATAL: Received SIGPOLL: Pollable event occurred (System V)." << log::endl;
        break;
    case SIGXCPU:
        log::error << "FATAL: Received SIGXCPU: CPU time limit exceeded." << log::endl;
        break;
    case SIGXFSZ:
        log::error << "FATAL: Received SIGXFSZ: File size limit exceeded." << log::endl;
        break;
    case SIGVTALRM:
        log::error << "FATAL: Received SIGVTALRM: Virtual timer expired." << log::endl;
        break;
    case SIGPROF:
        log::error << "FATAL: Received SIGPROF: Profiling timer expired." << log::endl;
        break;
    case SIGUSR1:
        log::error << "FATAL: Received SIGUSR1: User-defined signal 1." << log::endl;
        break;
    case SIGUSR2:
        log::error << "FATAL: Received SIGUSR2: User-defined signal 2." << log::endl;
        break;
    case SIGWINCH:
        log::error << "FATAL: Received SIGWINCH: Window size change (4.3 BSD, Sun)." << log::endl;
        break;
    default:
        log::error << "FATAL: Received unknown signal " << signo << log::endl;
        break;
    }
    back_trace(signo);
}

void Logger::back_trace(int signo)
{
    if (instance_) {
        auto logger = log::error << "Backtrace:\e[0m\n";
        logger << "    commit head = " << common::get_version() << "\n";

        constexpr size_t MaxTraceDepth = 128;
        void* trace_ptrs[MaxTraceDepth] = {0};
        auto trace_length = backtrace(trace_ptrs, MaxTraceDepth);
        auto trace_strings = backtrace_symbols(trace_ptrs, trace_length);

        constexpr size_t MaxFileName = 1024;
        char trace_fileline[MaxFileName] = {0};

        for (auto i = 2; i < trace_length; i++) {
            std::string trace = trace_strings[i];
            logger << "\n--> " << trace << "\n";

            auto sep_pos = trace.find('(');
            auto obj_name = trace.substr(0, sep_pos);
            auto left_pos = trace.find('[');
            auto right_pos = trace.find(']');
            auto offset_str = trace.substr(left_pos + 1, right_pos - left_pos - 1);

            if (obj_name.size() > 0 && obj_name[0] != '/') {
                obj_name = "`which " + obj_name + "`";
            }
            std::string cmd_line = "addr2line -Cifp -e " + obj_name + " " + offset_str;

            if (auto fp = popen(cmd_line.c_str(), "r")) {
                auto result = fgets(trace_fileline, sizeof(trace_fileline), fp);
                if (result != nullptr) {
                    if (strcmp(trace_fileline, "?? ??:0\n") != 0) {
                        logger << "    = " << trace_fileline;
                    }
                } else {
                    logger << "    = Can not determine file line\n";
                }
                pclose(fp);
            }
        }

        logger << log::endl;
        free(trace_strings);
        if (signal_to_ignore_user_.count(signo) == 0) {
            instance_.reset();
        }
    }
    if (signal_to_ignore_user_.count(signo) == 0) {
        log::error << "EXITING" << log::endl;
        exit(-1);
    }
}

}  // namespace impl
}  // namespace log
}  // namespace hera
}  // namespace wayz