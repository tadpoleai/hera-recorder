#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "generator.hpp"

namespace wayz {
namespace hera {
namespace device {
namespace parameter {

std::ostream& operator<<(std::ostream& os, const ParamDef& self)
{
    switch (self.category) {
    case ParamDef::Enum:
        os << "Enum " << self.designator << ", default = " << self.default_value << ", options = " << self.options;
        break;
    case ParamDef::Numeric:
        os << "Numeric " << self.type << " " << self.designator << ", default = " << self.default_value << ", range = ["
           << self.range_min << ", " << self.range_max << "]";
        break;
    case ParamDef::String:
        os << "String " << self.designator << ", default = " << self.default_value << ", regex = /" << self.regex
           << "/";
        break;
    case ParamDef::Boolean:
        os << "Boolean " << self.designator << ", default = " << self.default_value;
        break;
    }
    os << " Requires: " << self.requirement;
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<ParamDef>& self)
{
    os << "\n";
    for (auto& p : self) {
        os << p << "\n";
    }
    return os;
}

bool parse(const std::string& input_filename, std::string& escaped_file_content, std::vector<ParamDef>& param_defs)
{
    static const std::regex empty_pattern{R"(^\s*$)"};

    ///
    /// Comment
    ///
    static const std::regex comment_pattern{R"(^#\s*(.*)$)"};

    ///
    /// Expression
    ///
    static const std::regex requires_pattern{
            R"R(^requires\s(.*)$)R"};

    ///
    /// Name Default
    ///
    static const std::regex boolean_pattern{
            R"R(^(immutable|mutable)\s+boolean\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*(true|false|TRUE|FALSE)\s*;?\s*$)R"};

    ///
    /// Name Default Options
    ///
    static const std::regex enum_pattern{
            R"R(^(immutable|mutable)\s+enum\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*([a-zA-Z][a-zA-Z0-9]*)\s*;\s*options\s+(([a-zA-Z][a-zA-Z0-9]*\s*,\s*)+([a-zA-Z][a-zA-Z0-9]*))$)R"};

    ///
    /// Type Name Default RangeMin RangeMax
    ///
    static const std::regex numeric_pattern{
            R"R(^(immutable|mutable)\s+(real|integer)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*([0-9.+-eE]+)\s*;\s*range\s+([0-9.+-eE]+)\s*,\s*([0-9.+-eE]+)\s*,\s*([0-9.+-eE]+)$)R"};

    ///
    /// Name Default Regex
    ///
    static const std::regex string_pattern{
            R"R(^(immutable|mutable)\s+string\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*"(.*)"\s*;\s*regex (.*)$)R"};

    std::ifstream input_file;
    input_file.open(input_filename, std::ios::in);

    std::string line;
    std::string param_comment;
    std::string requires_string;

    if (input_file.is_open()) {
        size_t line_num = 0;
        while (getline(input_file, line)) {
            escaped_file_content += "/// " + line + "\n";
            line_num++;

            std::smatch matches;
            if (std::regex_search(line, matches, empty_pattern)) {
                param_comment.clear();
                requires_string.clear();
            } else if (std::regex_search(line, matches, comment_pattern)) {
                if (!param_comment.empty()) {
                    param_comment += "\n";
                }
                param_comment += matches[1].str();
            } else if (std::regex_search(line, matches, requires_pattern)) {
                requires_string = matches[1].str();
            } else if (std::regex_search(line, matches, boolean_pattern)) {
                auto comment = param_comment;
                param_defs.push_back({.category = ParamDef::Boolean,
                                      .type = "bool",
                                      .designator = matches[2].str(),
                                      .default_value = matches[3].str(),
                                      .comment = comment,
                                      .is_mutable = matches[1].str() == "mutable",
                                      .requirement = requires_string});
                param_comment.clear();
                requires_string.clear();
            } else if (std::regex_search(line, matches, enum_pattern)) {
                auto comment = param_comment;
                param_defs.push_back({.category = ParamDef::Enum,
                                      .type = matches[2].str(),
                                      .designator = matches[2].str(),
                                      .default_value = matches[3].str(),
                                      .comment = comment,
                                      .is_mutable = matches[1].str() == "mutable",
                                      .requirement = requires_string,
                                      .options = matches[4].str()});
                param_comment.clear();
                requires_string.clear();
            } else if (std::regex_search(line, matches, numeric_pattern)) {
                auto comment = param_comment;
                param_defs.push_back({.category = ParamDef::Numeric,
                                      .type = ((matches[2].str() == "real") ? "double" : "int"),
                                      .designator = matches[3].str(),
                                      .default_value = matches[4].str(),
                                      .comment = comment,
                                      .is_mutable = matches[1].str() == "mutable",
                                      .requirement = requires_string,
                                      .options = {},
                                      .range_min = matches[5].str(),
                                      .range_max = matches[6].str(),
                                      .range_step = matches[7].str()});
                param_comment.clear();
                requires_string.clear();
            } else if (std::regex_search(line, matches, string_pattern)) {
                auto comment = param_comment;
                param_defs.push_back({.category = ParamDef::String,
                                      .type = "std::string",
                                      .designator = matches[2].str(),
                                      .default_value = matches[3].str(),
                                      .comment = comment,
                                      .is_mutable = matches[1].str() == "mutable",
                                      .requirement = requires_string,
                                      .options = {},
                                      .range_min = {},
                                      .range_max = {},
                                      .range_step = {},
                                      .regex = matches[4].str()});
                param_comment.clear();
                requires_string.clear();
            } else {
                std::cerr << input_filename << ":" << line_num << ": error: Can not tokenize line\n";
                std::cerr << line << std::endl;
                return false;
            }
        }
        input_file.close();
    } else {
        std::cerr << input_filename << ": error: Can open file" << std::endl;
        return false;
    }

    return true;
}

}  // namespace parameter
}  // namespace device
}  // namespace hera
}  // namespace wayz