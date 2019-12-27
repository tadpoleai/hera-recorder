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

#include "common/logger/logger.hpp"
#include "common/utils/folder_content.hpp"

namespace wayz {
namespace hera {
namespace storage {

constexpr std::array<char, 16> StorageDataHeader::Magic = {"HERA_STORAGE_V3"};

StorageDataHeader::StorageDataHeader() : timestamp_start(0), timestamp_end(0) {}

StorageDataHeaderPtr StorageDataHeader::read_from(std::ifstream& ifs)
{
    try {
        std::remove_const_t<decltype(Magic)> magic;
        ifs.read(magic.data(), magic.size());
        if (ifs.gcount() != sizeof(magic)) {
            throw std::runtime_error("Can not read magic");
        }

        if (magic != Magic) {
            throw std::runtime_error("Magic does not match. This does not appear to be a wayz-hera record file");
        }

        auto header = std::make_shared<StorageDataHeader>();

        ifs.read((char*)&header->timestamp_start, sizeof(header->timestamp_start));
        if (ifs.gcount() != sizeof(header->timestamp_start)) {
            throw std::runtime_error("Can not read");
        }
        ifs.read((char*)&header->timestamp_end, sizeof(header->timestamp_end));
        if (ifs.gcount() != sizeof(header->timestamp_end)) {
            throw std::runtime_error("Can not read");
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
        ofs.write((char*)Magic.data(), Magic.size());  // Magic;
        length_written += Magic.size();

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

        return length_written;

    } catch (std::exception& e) {
        log::error << "Storage: Error read from storage since " << e.what() << log::endl;
        return 0;
    }
}

std::ostream& operator<<(std::ostream& os, const StorageDataHeader& rhs)
{
    os << "- Hera Binary Storage V3\n";
    os << "\n";
    os << "- From " << time::Timestamp(rhs.timestamp_start) << "\n";
    os << "- To   " << time::Timestamp(rhs.timestamp_end) << "\n";
    os << "- time::Duration: " << time::Duration(rhs.timestamp_end - rhs.timestamp_start) << "\n";
    os << "\n";
    os << "- Containing data of " << rhs.device_names.size() << " devices:"
       << "\n";

    auto max_message_num = *std::max_element(rhs.device_message_nums.begin(), rhs.device_message_nums.end());
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

    return os;
}

}  // namespace storage
}  // namespace hera
}  // namespace wayz
