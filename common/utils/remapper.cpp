#include "remapper.hpp"

#include <fstream>
#include <iostream>

#include "../third_party/json.hpp"

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
            std::cout << el.key() << " > " << el.value().get<std::string>() << std::endl;
        }

    } catch (...) {
        std::cout << "Processer: Remapper can not read from " << json_filename << std::endl;
        remap_ = {};
    }
}

}  // namespace common
}  // namespace hera
}  // namespace wayz