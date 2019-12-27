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
    StorageDataHeader();

    ///
    /// @brief Read a StorageDataHeader from ifstream
    ///
    /// @param ifs ifstream to read from
    /// @return StorageDataHeaderPtr shared pointer to StorageDataHeader
    /// @return nullptr when either data read is invalid or ifs is closed/empty/ended
    static StorageDataHeaderPtr read_from(std::ifstream& ifs);

    ///
    /// @brief Write a StorageDataHeader to ofstream
    ///
    /// @param ofs ofstream to write to
    /// @return size_t size written to ofs
    /// @note this function blocks during writing, called in Storage's writing thread
    size_t write_to(std::ofstream& ofs) const;

public:
    static const std::array<char, 16> Magic;

public:
    uint64_t timestamp_start;
    uint64_t timestamp_end;
    std::vector<uint32_t> device_message_nums;
    std::vector<uint64_t> device_data_sizes;
    std::vector<std::string> device_names;
};

}  // namespace storage
}  // namespace hera
}  // namespace wayz