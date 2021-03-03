///
/// @file upload.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Wrapper upload transmission
/// @date 2020-05-13
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifdef HERA_COMPILE_IN_REPO
#include "common/include/third_party/enum.hpp"
#include "common/include/utils/folder_content.hpp"
#include "common/include/utils/process.hpp"
#else
#include <hera/common/third_party/enum.hpp>
#include <hera/common/utils/folder_content.hpp>
#include <hera/common/utils/process.hpp>
#endif


namespace wayz {
namespace hera {
namespace storage {
namespace upload {

///
/// @brief Configuration of a upload transmission
///
struct Config {
    std::string name;  ///< Name of file to upload

    std::string source;  ///< Local source file name, only one file a time

    std::string protocol;  ///< Protocol using for uploading to remote

    std::string destination;  ///< Destination of upload to remote

    std::string remark;  ///< Remark of remote destination

    std::map<std::string, std::string> params{};  ///< Optional, extra params of uploading protocol

    ///
    /// @brief Use compression
    ///
    /// @note Compression ratio on hera file is typically 1.12.
    /// thus it can save network data usage.
    ///
    /// @note Compression can be a bottleneck on transmission over local Giga LAN,
    /// since throughput of compression is limited by CPU performance.
    ///
    ///
    bool compress{false};
};

///
/// @brief Stage of tranmission
///
enum class Stage {
    Inited,      ///< Transmission inited
    InProgress,  ///< Transmission in progress
    Finished,    ///< Transmission finished
    Error        ///< Error occured
};

///
/// @brief Status of a upload transmission
///
struct Status {
    Stage stage{Stage::Inited};

    ///
    /// @brief total file size
    ///
    file::FileSize total_size{0};

    ///
    /// @brief processed file size
    ///
    file::FileSize processed_size{0};

    ///
    /// @brief literal human readable speed, given by rsync command line
    ///
    std::string speed_literal{"0MB/s"};

    ///
    /// @brief literal human readable estimated time of arrival, given by rsync command line
    ///
    std::string eta{"--:--:--"};

    ///
    /// @brief reason of error, if occured
    ///
    std::string error_reason{""};
};


///
/// @brief Manager of a upload transmission
///
class Transmission {
public:
    ///
    /// @brief Create a new Upload Transmission
    ///
    /// @param config
    /// @return std::unique_ptr<Transmission>
    ///
    static std::unique_ptr<Transmission> create(const Config& config);

    ///
    /// @brief Terminate uploading
    ///
    virtual void terminate(){};

    ///
    /// @brief Get if transmission is running
    ///
    inline bool running() const noexcept
    {
        return status_.stage == Stage::InProgress || status_.stage == Stage::Inited;
    }

    ///
    /// @brief Get if transmission errored
    ///
    inline bool errored() const noexcept
    {
        return status_.stage == Stage::Error;
    }

    ///
    /// @brief Get the config object
    ///
    /// @return Config
    ///
    inline Config get_config() const noexcept
    {
        return config_;
    }

    ///
    /// @brief Get the status object
    ///
    /// @return Status
    ///
    inline Status get_status() const noexcept
    {
        return status_;
    }

protected:
    Transmission() = default;

    ///
    /// @brief Set error with reason
    ///
    void set_error(const std::string& reason);

    Config config_;  ///< config of transmission

    Status status_;  ///< status of transmission

    /// Static Factory
public:
    using PluginCtor = std::function<Transmission*(const Config& config)>;

    struct PluginEntry {
        std::string name;
        PluginCtor ctor;
    };

    ///
    /// @brief Load upload plugins(dynamic libraries)
    ///
    /// @param plugins_path path to dynamic libraries
    static void load_plugins(const std::string& plugins_path = "/usr/local/lib/hera/plugin");

private:
    static std::vector<PluginEntry> plugin_entries;  ///< Registered children
    static std::mutex load_mutex;
    static bool is_loaded;
};

}  // namespace upload
}  // namespace storage
}  // namespace hera
}  // namespace wayz
