///
/// @file process.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Handler of process and pipe
/// @date 2020-05-13
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <string>
#include <vector>

namespace wayz {
namespace hera {
namespace common {

///
/// @brief Manager of a process along with pipe attached on it
///
class Process final {
public:
    ///
    /// @brief Construct a new Process object
    ///
    /// @param argv argv of execvp
    ///
    Process(const std::vector<std::string>& argv);

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    Process(Process&&) = default;

    ///
    /// @brief Destroy the Process object
    ///
    /// @note close pipe here
    ///
    ~Process();

    inline ::pid_t get_pid() const noexcept
    {
        return pid_;
    }

    inline ::FILE* get_stdout() const noexcept
    {
        return out_;
    }

    inline ::FILE* get_stderr() const noexcept
    {
        return err_;
    }

    inline ::FILE* get_stdin() const noexcept
    {
        return in_;
    }

    ///
    /// @brief Send SIGTERM to process
    ///
    void terminate();

    ///
    /// @brief Send SIGKILL to process
    ///
    void kill();


private:
    ///
    /// @brief pid of process
    ///
    ::pid_t pid_;

    ///
    /// @brief pipe read from stdout of process
    ///
    ::FILE* out_;

    ///
    /// @brief pipe read from stderr of process
    ///
    ::FILE* err_;

    ///
    /// @brief pipe write to stdin of process
    ///
    ::FILE* in_;
};

}  // namespace common
}  // namespace hera
}  // namespace wayz