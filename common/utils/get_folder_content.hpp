//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <string>
#include <vector>

namespace wayz {
namespace tron {

struct FolderContent {
    bool opened;
    std::vector<std::string> files;
    std::vector<std::string> folders;
    FolderContent() : opened(false) {}
};

FolderContent get_folder_content(const std::string& parent);

}  // namespace tron
}  // namespace wayz