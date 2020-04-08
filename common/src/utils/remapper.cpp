//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include <fstream>
#include <iostream>

#include "third_party/json.hpp"
#include "common/include/logger/logger.hpp"
#include "utils/remapper.hpp"

namespace wayz {
namespace hera {
namespace common {

using json = nlohmann::json;

Remapper::Remapper(const std::string& json_filename)
{
    json json_instance;
    try {
        std::ifstream ifs;
        ifs.open(json_filename, std::ios::in);
        ifs >> json_instance;
        ifs.close();

        for (const auto& el : json_instance.items()) {
            remap_[el.key()] = el.value().get<std::string>();
            log::debug << "Remapper: remapping from '" << el.key() << "' to '" << el.value().get<std::string>() << "'"
                       << log::endl;
        }

    } catch (...) {
        log::error << "Remapper: can not read from '" << json_filename << "'" << log::endl;
        remap_ = {};
    }
}

}  // namespace common
}  // namespace hera
}  // namespace wayz