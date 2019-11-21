//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace wayz {
namespace hera {

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
    FileAttribute(const std::string& basename,
                  const std::string& fullname,
                  const FileSize size = 0) :
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

FolderContent get_folder_content(const std::string& parent);

}  // namespace hera
}  // namespace wayz