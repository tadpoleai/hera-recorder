//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "utils/folder_content.hpp"

#include <cstring>
#include <dirent.h>
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>

namespace wayz {
namespace hera {
namespace file {

std::ostream& operator<<(std::ostream& os, const FileSize& size)
{
    if (size.size < (1ULL << 10)) {
        os << std::setw(4) << std::setfill(' ') << size.size << "Bytes";
    } else {
        os << std::setw(6) << std::setprecision(5) << std::setfill('0') << std::left;
        if (size.size < (1ULL << 20)) {
            os << size.size / float(1ULL << 10) << "KiB";
        } else if (size.size < (1ULL << 30)) {
            os << size.size / float(1ULL << 20) << "MiB";
        } else if (size.size < (1ULL << 40)) {
            os << size.size / float(1ULL << 30) << "GiB";
        } else {
            os << size.size / float(1ULL << 40) << "TiB";
        }
    }
    return os;
}

FileSize get_file_size(const std::string& filename)
{
    struct stat stat_result;
    if (stat(filename.c_str(), &stat_result)) {
        return 0;
    }

    if (S_ISREG(stat_result.st_mode)) {
        return stat_result.st_size;
    } else {
        return 0;
    }
}

FolderContent get_folder_content(const std::string& parent)
{
    FolderContent content;
    auto dir = opendir(parent.c_str());
    if (!dir) {
        return content;
    }

    content.opened = true;
    while (auto node = readdir(dir)) {
        if (strcmp(node->d_name, ".") == 0 || strcmp(node->d_name, "..") == 0) {
            continue;
        }

        auto node_fullname = parent + "/" + node->d_name;
        struct stat stat_result;
        if (stat(node_fullname.c_str(), &stat_result)) {
            continue;
        }

        if (S_ISREG(stat_result.st_mode)) {
            content.total_size = content.total_size + stat_result.st_size;
            content.files.emplace_back(FileAttribute(node->d_name, node_fullname, stat_result.st_size));
        } else if (S_ISDIR(stat_result.st_mode)) {
            content.folders.emplace_back(FileAttribute(node->d_name, node_fullname));
        }
    }
    closedir(dir);

    return content;
}

FilesystemStatus get_filesystem_status(const std::string& path)
{
    FilesystemStatus ret;
    struct statfs diskInfo;

    if (statfs(path.c_str(), &diskInfo) < 0) {
        ret.opened = false;
        return ret;
    } else {
        ret.opened = true;
        uint64_t blocksize = diskInfo.f_bsize;
        ret.total_space = diskInfo.f_blocks * blocksize;
        ret.free_space = diskInfo.f_bavail * blocksize;
        ret.used_space = ret.total_space - ret.free_space;
    }
    return ret;
}

}  // namespace file
}  // namespace hera
}  // namespace wayz