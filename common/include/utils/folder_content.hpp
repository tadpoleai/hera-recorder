//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wayz {
namespace hera {
namespace file {

struct FileSize {
    friend std::ostream& operator<<(std::ostream& os, const FileSize& size);

public:
    uint64_t size;
    FileSize(uint64_t size) : size(size) {}
    operator uint64_t() const
    {
        return size;
    }
};

struct FileAttribute {
    std::string basename;
    std::string fullname;
    FileSize size;
    FileAttribute(const std::string& basename, const std::string& fullname, const FileSize size = 0) :
        basename(basename),
        fullname(fullname),
        size(size)
    {}
};

struct FolderContent {
    bool opened;
    FileSize total_size;
    std::vector<FileAttribute> files;
    std::vector<FileAttribute> folders;
    FolderContent() : opened(false), total_size(0) {}
};

struct FilesystemStatus {
    bool opened;
    FileSize total_space;
    FileSize free_space;
    FileSize used_space;
    FilesystemStatus() : opened(false), total_space(0), free_space(0), used_space(0) {}
};

FolderContent get_folder_content(const std::string& parent);

FilesystemStatus get_filesystem_status(const std::string& path);

}  // namespace file
}  // namespace hera
}  // namespace wayz