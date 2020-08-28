///
/// @file parameter.hpp
/// @author lyu zheming (zheming.lyu@wayz.ai)
/// @brief Parameter for devices
/// @date 2020-08-06
///
/// @copyright Copyright 2018 Wayz.ai. All Rights Reserved.
///

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#ifdef HERA_COMPILE_IN_REPO
#include "common/include/third_party/json.hpp"
#else
#include <hera/common/third_party/json.hpp>
#endif

namespace wayz {
namespace hera {
namespace device {

class ParametersInterface {
public:
    ///
    /// @brief Set parameters by json
    ///
    /// @param json_input parameters in json
    /// @return true parameter is set
    /// @return false parameter no set, type error
    virtual bool set(const nlohmann::json& json_input) = 0;

    ////
    /// @brief Set parameters by type, value
    ///
    /// @param type type in string
    /// @param value value in string
    /// @return true parameter is set
    /// @return false parameter no set, type error
    virtual bool set(const std::string& type, const std::string& value) = 0;

    ///
    /// @brief Dump parameters into json
    ///
    /// @return nlohmann::json
    nlohmann::json dump() const noexcept
    {
        return json_values_;
    }

    ///
    /// @brief Check if parameters is good
    ///
    /// @param [out] reason if no good, reason is set
    /// @return true parameters is good
    /// @return false parameters is ill-formed / empty
    bool check(std::string& reason)
    {
        return true;
    }

    ///
    /// @brief Return rules of parameter in json
    ///
    /// @return std::string rules in json, format below:
    /*
    [
        {
            { type : "enum"/"string"/"int"/"double" },
            { default: },
            { range: { min: MIN_V, max: MAX_V },            # only for numeric
            { options: ["ENUM_VAL_0", "ENUM_VAL_1", ... ] } # only for enum
            { regex: "REGEX" }                              # only for string
        }, ...
    ]
     */
    virtual std::string rules() const = 0;

protected:
    ParametersInterface() = default;

protected:
    nlohmann::json json_values_{};
};

}  // namespace device
}  // namespace hera
}  // namespace wayz