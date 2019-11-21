#pragma once

#include <memory>
#include <unordered_map>

namespace wayz {
namespace hera {

class Remapper;
using RemapperPtr = std::unique_ptr<Remapper>;

class Remapper {
public:
    static auto create(const std::string& json_filename)
    {
        return RemapperPtr(new Remapper(json_filename));
    }

    inline std::string remap(std::string&& in_name) const
    {
        auto search = remap_.find(in_name);
        if (search != remap_.end()) {
            return search->second;
        } else {
            return std::forward<std::string>(in_name);
        }
    }

private:
    Remapper(const std::string& json_filename);

private:
    std::unordered_map<std::string, std::string> remap_;
};

}  // namespace hera
}  // namespace wayz