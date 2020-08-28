//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#pragma once

#include <string>
#include <vector>

#include "common/include/third_party/json.hpp"
#include "daemon/rpc/gen-cpp/Service.h"

namespace wayz {
namespace hera {
namespace daemon {

using json = nlohmann::json;

void to_json(json& j, const Parameter& s);
void to_json(json& j, const Device& s);
void to_json(json& j, const Profile& s);
void to_json(json& j, const OperatorInfo& s);
void to_json(json& j, const AcquisitionSetting& s);

void from_json(const json& j, Parameter& s);
void from_json(const json& j, Device& s);
void from_json(const json& j, Profile& s);
void from_json(const json& j, OperatorInfo& s);
void from_json(const json& j, AcquisitionSetting& s);

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
