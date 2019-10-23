//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "get_folder_content.hpp"

#include <cstring>
#include <dirent.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

namespace wayz {
namespace tron {

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
        if (stat((node_fullname).c_str(), &stat_result)) {
            continue;
        }

        if (S_ISREG(stat_result.st_mode)) {
            content.files.emplace_back(node_fullname);
        } else if (S_ISDIR(stat_result.st_mode)) {
            content.folders.emplace_back(node_fullname);
        }
    }
    closedir(dir);

    return content;
}

}  // namespace tron
}  // namespace wayz