///
/// @file storage_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class StorageDataHeader
/// @version 0.1
/// @date 2019-12-24
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <ostream>
#include <vector>

#ifdef HERA_COMPILE_IN_REPO
#include "common/include/logger/log_string.hpp"
#include "common/include/third_party/json.hpp"
#else
#include <hera/common/logger/log_string.hpp>
#include <hera/common/third_party/json.hpp>
#endif

namespace wayz {
namespace hera {
namespace storage {

class StorageDataHeader;

///
/// @brief Shared pointer to StorageDataHeader
///
using StorageDataHeaderPtr = std::shared_ptr<StorageDataHeader>;

///
/// @brief StorageDataHeader in file
///
class StorageDataHeader {
    friend std::ostream& operator<<(std::ostream& os, const StorageDataHeader& rhs);

public:
    ///
    /// @brief Construct a new Storage Data object
    ///
    StorageDataHeader(const int32_t version, const bool is_extra = true, const bool is_logs = true);

    ///
    /// @brief Read a StorageDataHeader from ifstream
    ///
    /// @param ifs ifstream to read from
    /// @param is_extra bool read extra information
    /// @param is_logs bool read logs
    ///
    /// @return StorageDataHeaderPtr shared pointer to StorageDataHeader
    /// @return nullptr when either data read is invalid or ifs is closed/empty/ended
    ///
    /// @note must check whether return value is nullptr
    static StorageDataHeaderPtr read_from(std::ifstream& ifs, const bool is_extra = true, const bool is_logs = true);

    ///
    /// @brief Write a StorageDataHeader to ofstream
    ///
    /// @param ofs ofstream to write to
    /// @return size_t size written to ofs
    /// @note this function blocks during writing, called in Storage's writing thread
    size_t write_to(std::ofstream& ofs) const;

public:
    ///
    /// @brief Magic Header of Storage Version V3
    ///
    static const std::array<char, 16> MagicV3;

    ///
    /// @brief Magic Header of Storage Version V4
    ///
    static const std::array<char, 16> MagicV4;

    ///
    /// @brief Max Header Length in V4
    ///
    static constexpr size_t ReservedLength = (4 << 20);

public:
    ///
    /// @brief version of hera storage
    /// @note V3 = only contains Common Info
    /// @note V4 = additional json info
    ///
    const int32_t Version;

    ///
    /// @brief If extra info is read into header
    ///
    const bool IsExtra;

    ///
    /// @brief If logs is read into header
    ///
    const bool IsLogs;

    ///
    /// @defgroup commonInfo Common Info in V3
    ///

    ///
    /// @addtogroup commonInfo
    /// @brief timestamp when record starts
    ///
    uint64_t timestamp_start;

    ///
    /// @addtogroup commonInfo
    /// @brief timestamp when record stops
    ///
    uint64_t timestamp_end;

    ///
    /// @addtogroup commonInfo
    /// @brief message nums of devices
    ///
    std::vector<uint32_t> device_message_nums;

    ///
    /// @addtogroup commonInfo
    /// @brief data sizes in bytes of devices
    ///
    std::vector<uint64_t> device_data_sizes;

    ///
    /// @addtogroup commonInfo
    /// @brief data sizes in bytes of devices
    ///
    std::vector<std::string> device_names;

    ///
    /// @brief Extra informations
    ///
    nlohmann::json extra_info;

    ///
    /// @brief Logs
    ///
    std::vector<log::impl::LogString> logs;
};

}  // namespace storage
}  // namespace hera
}  // namespace wayz