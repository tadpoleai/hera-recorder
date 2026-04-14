///
/// @file device_data.hpp
/// @author zheming.lyu (zheming.lyu@wayz.ai)
/// @brief Class StorageDataHeader
/// @version 0.1
/// @date 2019-12-24
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///


#include "storage_data_header.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <fstream>
#include <iomanip>

#include "common/include/logger/logger.hpp"
#include "common/include/utils/folder_content.hpp"

namespace wayz {
namespace hera {
namespace storage {

constexpr std::array<char, 16> StorageDataHeader::MagicV3 = {"HERA_STORAGE_V3"};

constexpr std::array<char, 16> StorageDataHeader::MagicV4 = {"HERA_STORAGE_V4"};

constexpr std::array<char, 32> StorageDataHeader::MagicIndices = {  //
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    'I',
    'N',
    'D',
    'X',
    23,
    static_cast<char>(-19),
    20,
    13,
    static_cast<char>(-4),
    4,
    static_cast<char>(-4),
    1,
    static_cast<char>(-10),
    4,
    9,
    22};

StorageDataHeader::StorageDataHeader(const int32_t version, const bool is_extra, const bool is_logs) :
    Version(version),
    IsExtra(is_extra || is_logs),
    IsLogs(is_logs),
    timestamp_start(0),
    timestamp_end(0),
    extra_info(nlohmann::json::parse("{}"))
{}

StorageDataHeaderPtr StorageDataHeader::read_from(std::ifstream& ifs, const bool is_extra, const bool is_logs)
{
    try {
        std::remove_const_t<decltype(MagicV3)> magic;
        ifs.read(magic.data(), magic.size());
        if (ifs.gcount() != sizeof(magic)) {
            throw std::runtime_error("Can not read magic");
        }

        int32_t version = 0;
        if (magic == MagicV3) {
            version = 3;
        } else if (magic == MagicV4) {
            version = 4;
        } else {
            throw std::runtime_error("Magic does not match. This does not appear to be a wayz-hera record file");
        }

        auto header = std::make_shared<StorageDataHeader>(version, is_extra, is_logs);

        ifs.read((char*)&header->timestamp_start, sizeof(header->timestamp_start));
        if (ifs.gcount() != sizeof(header->timestamp_start)) {
            throw std::runtime_error("Can not read");
        }
        ifs.read((char*)&header->timestamp_end, sizeof(header->timestamp_end));
        if (ifs.gcount() != sizeof(header->timestamp_end)) {
            throw std::runtime_error("Can not read");
        }
        if (header->timestamp_end == 0) {
            log::error << "Storage: timestamp_end invalid, storage header is broken. \n\t Run `hera-storage-tool -i "
                          "<storage_file> -b` to rebuild header";
        }

        uint32_t device_num;
        ifs.read((char*)&device_num, sizeof(device_num));
        if (ifs.gcount() != sizeof(device_num)) {
            throw std::runtime_error("Can not read");
        }

        for (uint32_t i = 0; i < device_num; i++) {
            uint32_t device_message_num;
            ifs.read((char*)&device_message_num, sizeof(device_message_num));
            if (ifs.gcount() != sizeof(device_message_num)) {
                throw std::runtime_error("Can not read");
            }
            header->device_message_nums.emplace_back(device_message_num);
        }

        for (uint32_t i = 0; i < device_num; i++) {
            uint64_t device_data_size;
            ifs.read((char*)&device_data_size, sizeof(device_data_size));
            if (ifs.gcount() != sizeof(device_data_size)) {
                throw std::runtime_error("Can not read");
            }
            header->device_data_sizes.emplace_back(device_data_size);
        }

        for (uint32_t i = 0; i < device_num; i++) {
            uint32_t device_name_length;
            ifs.read((char*)&device_name_length, sizeof(device_name_length));
            if (ifs.gcount() != sizeof(device_name_length)) {
                throw std::runtime_error("Can not read");
            }

            std::string device_name;
            device_name.resize(device_name_length);
            ifs.read((char*)device_name.data(), device_name_length);
            if (ifs.gcount() != device_name_length) {
                throw std::runtime_error("Can not read");
            }
            header->device_names.emplace_back(std::move(device_name));
        }

        if (version == 3) {
            return header;
        }

        // V4
        uint32_t extra_info_size = 0;
        ifs.read((char*)&extra_info_size, sizeof(extra_info_size));
        if (ifs.gcount() != sizeof(extra_info_size)) {
            throw std::runtime_error("Can not read extra info size");
        }
        if (extra_info_size > ReservedLength) {
            throw std::runtime_error("Invalid Extra Info Size");
        }

        std::string extra_info_str;
        extra_info_str.resize(extra_info_size);
        if (is_extra && extra_info_size > 0) {
            ifs.read((char*)(extra_info_str.data()), extra_info_size);
            header->extra_info = nlohmann::json::parse(extra_info_str);
        } else {
            ifs.seekg(extra_info_size, std::ios::cur);
        }

        // Read Log Info
        while (is_logs) {
            log::impl::LogString log_string;
            ifs.read((char*)(&log_string.level), sizeof(log_string.level));
            if (ifs.gcount() != sizeof(log_string.level)) {
                throw std::runtime_error("Can not read log");
            }
            if (log::LogLevel::Reserved == log_string.level) {
                log::info << "End of log in hera file" << log::endl;
                break;
            }

            uint64_t ts;
            ifs.read((char*)(&ts), sizeof(ts));
            if (ifs.gcount() != sizeof(ts)) {
                throw std::runtime_error("Can not read log");
            }
            log_string.ts = ts;

            int32_t str_length;
            ifs.read((char*)(&str_length), sizeof(str_length));
            if (ifs.gcount() != sizeof(str_length)) {
                throw std::runtime_error("Can not read log");
            }

            log_string.str.resize(str_length);
            ifs.read((char*)log_string.str.data(), str_length);
            if (ifs.gcount() != str_length) {
                throw std::runtime_error("Can not read log");
            }

            header->logs.emplace_back(std::move(log_string));
        }

        // Read Indices
        // Read Magic and length
        uint64_t number_of_indices = 0;
        std::remove_const_t<decltype(MagicIndices)> magicIndices;

        ifs.seekg(ReservedLength - sizeof(MagicIndices) - sizeof(uint64_t), std::ios::beg);
        ifs.read((char*)&number_of_indices, sizeof(number_of_indices));
        if (ifs.gcount() != sizeof(number_of_indices)) {
            throw std::runtime_error("Size of file is less than expected");
        }
        ifs.read(magicIndices.data(), magicIndices.size());
        if (ifs.gcount() != sizeof(magicIndices)) {
            throw std::runtime_error("Size of file is less than expected");
        }
        if (magicIndices == MagicIndices && number_of_indices > 0 &&
            number_of_indices < ReservedLength / sizeof(StorageHeaderTimestampIndex)) {

            log::debug << "Reading Indices" << log::endl;
            ifs.seekg(ReservedLength - sizeof(MagicIndices) - sizeof(uint64_t) -
                              number_of_indices * sizeof(StorageHeaderTimestampIndex),
                      std::ios::beg);
            header->indices.resize(number_of_indices);
            ifs.read((char*)header->indices.data(), number_of_indices * sizeof(StorageHeaderTimestampIndex));
            if ((int64_t)ifs.gcount() != (int64_t)(number_of_indices * sizeof(StorageHeaderTimestampIndex))) {
                throw std::runtime_error("Size of file is less than expected");
            }
        }

        ifs.seekg(ReservedLength);

        return header;
    } catch (std::exception& e) {
        log::error << "Storage: Error read from storage. " << e.what() << log::endl;
        return nullptr;
    }
}

size_t StorageDataHeader::write_to(std::ofstream& ofs) const
{
    try {
        uint32_t length_written = 0;
        if (Version == 3) {
            ofs.write((char*)MagicV3.data(), MagicV3.size());  // Magic;
            length_written += MagicV3.size();
        } else if (Version == 4) {
            ofs.write((char*)MagicV4.data(), MagicV4.size());  // Magic;
            length_written += MagicV4.size();
        } else {
            throw std::runtime_error("Invalid Storage Version");
        }

        ofs.write((char*)&timestamp_start, sizeof(timestamp_start));
        length_written += sizeof(timestamp_start);
        ofs.write((char*)&timestamp_end, sizeof(timestamp_end));
        length_written += sizeof(timestamp_end);

        uint32_t device_num = device_names.size();
        ofs.write((char*)&device_num, sizeof(device_num));  // Device num
        length_written += sizeof(device_num);

        for (uint32_t i = 0; i < device_num; i++) {
            ofs.write((char*)&device_message_nums[i], sizeof(device_message_nums[i]));  // Device message num
            length_written += sizeof(device_message_nums[i]);
        }

        for (uint32_t i = 0; i < device_num; i++) {
            ofs.write((char*)&device_data_sizes[i], sizeof(device_data_sizes[i]));  // Device data size
            length_written += sizeof(device_data_sizes[i]);
        }

        for (uint32_t i = 0; i < device_num; i++) {
            uint32_t device_name_length = device_names[i].size();
            ofs.write((char*)&device_name_length, sizeof(device_name_length));  // Device name length
            length_written += sizeof(device_name_length);
            ofs.write((char*)device_names[i].data(), device_name_length);  // Device name
            length_written += device_name_length;
        }

        if (Version == 3) {
            return length_written;
        }

        // Write V4

        // Write Extra Info
        if (IsExtra) {
            std::string extra_info_str = extra_info.dump(-1, 0, false, nlohmann::json::error_handler_t::ignore);
            uint32_t extra_info_size = extra_info_str.size();
            if (extra_info_str.size() + length_written > ReservedLength) {
                log::error << "Storage: Too long extra info when calling write_to" << log::endl;
                extra_info_size = ReservedLength;
                ofs.write((char*)&extra_info_size, sizeof(extra_info_size));
                ofs.seekp(ReservedLength, std::ios::beg);
                return length_written;
            }
            ofs.write((char*)&extra_info_size, sizeof(extra_info_size));
            ofs.write((char*)extra_info_str.data(), extra_info_size);
            length_written += extra_info_size + sizeof(extra_info_size);
        } else {
            uint32_t extra_info_size = 0;
            ofs.write((char*)&extra_info_size, sizeof(extra_info_size));
            length_written += sizeof(extra_info_size);
        }

        // Calculate Indices size;
        const uint64_t number_of_indices = indices.size();
        size_t indices_reserve_offset = ReservedLength - sizeof(MagicIndices) - sizeof(uint64_t) -
                                        sizeof(StorageHeaderTimestampIndex) * number_of_indices;
        if (IsLogs) {
            // Write Log Info
            for (const auto& log_string : logs) {
                uint32_t log_string_size = log_string.str.size();

                if (log_string_size + length_written + 64 > indices_reserve_offset) {
                    uint64_t Zero = 0;
                    ofs.write((char*)(&Zero), sizeof(Zero));
                    break;
                }

                ofs.write((char*)(&log_string.level), sizeof(log_string.level));
                length_written += sizeof(log_string.level);

                uint64_t ts = log_string.ts;
                ofs.write((char*)(&ts), sizeof(ts));
                length_written += sizeof(ts);

                ofs.write((char*)(&log_string_size), sizeof(log_string_size));
                ofs.write((char*)(log_string.str.data()), log_string_size);
                length_written += log_string_size + sizeof(log_string_size);
            }
        }

        // Write Indices
        ofs.seekp(indices_reserve_offset, std::ios::beg);
        ofs.write((char*)(indices.data()), sizeof(StorageHeaderTimestampIndex) * number_of_indices);
        ofs.write((char*)(&number_of_indices), sizeof(number_of_indices));
        ofs.write((char*)MagicIndices.data(), MagicIndices.size());  // Magic;

        ofs.seekp(ReservedLength, std::ios::beg);
        return ReservedLength;
    } catch (std::exception& e) {
        log::error << "Storage: Error write to storage since " << e.what() << log::endl;
        return 0;
    }
}

std::ostream& operator<<(std::ostream& os, const StorageDataHeader& rhs)
{
    if (rhs.Version == 3) {
        os << "- Hera Binary Storage V3\n";
    } else if (rhs.Version == 4) {
        os << "- Hera Binary Storage V4\n";
    } else {
        os << "- Hera Binary Storage V?\n";
    }

    os << "\n";
    os << "- From " << time::Timestamp(rhs.timestamp_start) << "\n";
    os << "- To   " << time::Timestamp(rhs.timestamp_end) << "\n";
    os << "- Duration: " << time::Duration(rhs.timestamp_end - rhs.timestamp_start) << "\n";
    os << "\n";
    os << "- Containing data of " << rhs.device_names.size() << " devices:"
       << "\n";

    auto max_message_num = 0;
    if (!rhs.device_message_nums.empty()) {
        max_message_num = *std::max_element(rhs.device_message_nums.begin(), rhs.device_message_nums.end());
    }

    uint64_t total_size = 0;

    for (size_t i = 0; i < rhs.device_names.size(); ++i) {
        os << "-   ID: " << std::right << std::setfill(' ') << std::setw(::log10(rhs.device_names.size()) + 1) << i
           << ", ";
        os << "Name: " << rhs.device_names[i] << "\n";
        os << "-     with " << std::right << std::setfill(' ') << std::setw(::log10(max_message_num + 1) + 1)
           << rhs.device_message_nums[i] << " Messages, ";
        os << "size = " << file::FileSize(rhs.device_data_sizes[i]) << ", ";
        os << "average frequency = "
           << rhs.device_message_nums[i] /
                        double(time::Duration(rhs.timestamp_end - rhs.timestamp_start) / time::OneSecond)
           << "Hz\n";
        total_size += rhs.device_data_sizes[i];
    }
    os << "\n";
    os << "- Total data size: " << file::FileSize(total_size) << "\n";

    if (rhs.IsExtra) {
        os << "\n- Extra Infos:\n";
        os << rhs.extra_info.dump(2) << "\n";
    }

    if (rhs.IsLogs) {
        os << "\n- Logs:\n";
        for (auto& log : rhs.logs) {
            os << "  " << log.level.to_color_prefix() << log::impl::Logger::format(log) << log.level.to_color_suffix()
               << "\n";
        }
    }

    if (!rhs.indices.empty()) {
        os << "\n- With " << rhs.indices.size() << " indices\n";
    } else {
        os << "\n- No indices available\n";
    }

    return os;
}

}  // namespace storage
}  // namespace hera
}  // namespace wayz
