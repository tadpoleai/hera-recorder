//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "serialize.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void to_json(json& j, const Parameter& s)
{
    j["type"] = s.type;
    j["value"] = s.value;
}
void to_json(json& j, const Device& s)
{
    j["type"] = s.type;
    j["name"] = s.name;
    j["parameters"] = s.parameters;
    j["forward"] = s.forward;
}
void to_json(json& j, const Profile& s)
{
    j["name"] = s.name;
    j["author"] = s.author;
    j["devices"] = s.devices;
}
void to_json(json& j, const OperatorInfo& s)
{
    j["operatorName"] = s.operatorName;
    j["place"] = s.place;
    j["slam"] = s.slam;
}
void to_json(json& j, const AcquisitionSetting& s)
{
    j["profileIndex"] = s.profileIndex;
    j["profiles"] = s.profiles;
    j["operatorInfo"] = s.operatorInfo;
}

void from_json(const json& j, Parameter& s)
{
    s.type = j.at("type");
    s.value = j.at("value");
}
void from_json(const json& j, Device& s)
{
    s.type = j.at("type");
    s.name = j.at("name");
    std::vector<Parameter> t = j.at("parameters");
    s.parameters = std::move(t);
    s.forward = j.at("forward");
}
void from_json(const json& j, Profile& s)
{
    s.name = j.at("name");
    s.author = j.at("author");
    std::vector<Device> t = j.at("devices");
    s.devices = std::move(t);
}
void from_json(const json& j, OperatorInfo& s)
{
    s.operatorName = j.at("operatorName");
    s.place = j.at("place");
    s.slam = j.at("slam");
}
void from_json(const json& j, AcquisitionSetting& s)
{
    s.profileIndex = j.at("profileIndex");
    std::vector<Profile> t = j.at("profiles");
    s.profiles = std::move(t);
    s.operatorInfo = j.at("operatorInfo");
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz
