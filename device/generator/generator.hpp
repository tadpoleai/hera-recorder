#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "unistd.h"

namespace wayz {
namespace hera {
namespace device {
namespace parameter {

struct ParamDef {
    friend std::ostream& operator<<(std::ostream& os, const ParamDef& self);

public:
    enum { Boolean, Enum, Numeric, String } category;

    std::string type;
    std::string designator;
    std::string label;
    std::string default_value;
    std::string comment;
    bool is_mutable;
    bool is_convert;

    std::string requirement{};
    std::string options{};
    std::string range_min{};
    std::string range_max{};
    std::string range_step{};
    std::string regex{};
};

std::ostream& operator<<(std::ostream& os, const std::vector<ParamDef>& self);

struct DeviceDesc {
    std::string label;
    std::string comment;
    std::vector<ParamDef> parameters;
};

bool parse(const std::string& input_filename, std::string& escaped_file_content, DeviceDesc& device_desc);

bool output(const std::string& output_filename,
            const std::string& escaped_file_content,
            const DeviceDesc& device_desc,
            const std::string& category_name,
            const std::string& vendor_name,
            const std::string& sub_name);

}  // namespace parameter
}  // namespace device
}  // namespace hera
}  // namespace wayz
