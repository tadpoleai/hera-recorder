//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "service.hpp"


namespace wayz {
namespace hera {
namespace daemon {

void Service::load_profiles()
{
    log::debug << "Daemon::load profiles called" << log::endl;
    json profile_outer_json;

    try {
        std::ifstream ifs;
        ifs.open(ProfilesFileName_, std::ios::in);
        ifs >> profile_outer_json;
        ifs.close();

        profiles_json_ = profile_outer_json["profiles"];
        profile_index_ = profile_outer_json["index"];

        for (const auto& profile_json : profiles_json_) {
            Profile profile;
            profile.name = profile_json["name"];
            profile.author = profile_json["author"];
            for (const auto& device_json : profile_json["devices"]) {
                Device device;
                device.type = device_json["type"];
                device.name = device_json["name"];
                device.forward = device_json["forward"];
                for (const auto& parameter_json : device_json["essentialParameters"]) {
                    Parameter parameter;
                    parameter.type = parameter_json["type"];
                    parameter.value = parameter_json["value"];
                    device.essentialParameters.emplace_back(parameter);
                }
                for (const auto& parameter_json : device_json["optionalParameters"]) {
                    Parameter parameter;
                    parameter.type = parameter_json["type"];
                    parameter.value = parameter_json["value"];
                    device.optionalParameters.emplace_back(parameter);
                }
                profile.devices.emplace_back(device);
            }
            profiles_.emplace_back(profile);
        }
    } catch (const std::exception& e) {
        try {
            std::ofstream ofs;
            profile_outer_json = json::parse("{}");
            profile_outer_json["index"] = 0;
            profile_outer_json["profiles"] = json::parse("[]");
            ofs.open(ProfilesFileName_, std::ios::out);
            ofs << profile_outer_json.dump(2);
            ofs.close();
            log::info << "Daemon::Created empty profiles";
        } catch (const std::exception& e) {
            log::error << e.what() << log::endl;
            log::error << "Daemon::Can not load profiles" << log::endl;
        }
    }

    log::debug << "Daemon::profiles loaded" << log::endl;
}

void Service::dump_profiles()
{
    log::debug << "Daemon::dump profiles called" << log::endl;

    try {
        json profile_outer_json = json::parse("{}");
        profiles_json_ = json::parse("[]");
        for (const auto& profile : profiles_) {
            json profile_json;
            profile_json["name"] = profile.name;
            profile_json["author"] = profile.author;
            profile_json["devices"] = json::parse("[]");
            for (const auto& device : profile.devices) {
                json device_json;
                device_json["type"] = device.type;
                device_json["name"] = device.name;
                device_json["forward"] = device.forward;
                device_json["essentialParameters"] = json::parse("[]");
                device_json["optionalParameters"] = json::parse("[]");
                for (const auto& parameter : device.essentialParameters) {
                    json parameter_json;
                    parameter_json["type"] = parameter.type;
                    parameter_json["value"] = parameter.value;
                    device_json["essentialParameters"].emplace_back(parameter_json);
                }
                for (const auto& parameter : device.optionalParameters) {
                    json parameter_json;
                    parameter_json["type"] = parameter.type;
                    parameter_json["value"] = parameter.value;
                    device_json["optionalParameters"].emplace_back(parameter_json);
                }
                profile_json["devices"].emplace_back(device_json);
            }
            profiles_json_.emplace_back(profile_json);
        }

        profile_outer_json["profiles"] = profiles_json_;
        profile_outer_json["index"] = profile_index_;

        std::ofstream ofs;
        ofs.open(ProfilesFileName_, std::ios::out);
        ofs << profile_outer_json.dump(2);
        ofs.close();
    } catch (const std::exception& e) {
        log::error << e.what() << log::endl;
        log::error << "Daemon::Can not dump profiles" << log::endl;
    }

    log::debug << "Daemon::profiles dumped" << log::endl;
}

void Service::updateProfiles(Result& result, const std::vector<Profile>& profiles, const int32_t profileIndex)
{
    log::info << "Daemon::updateProfile called" << log::endl;

    std::unique_lock<std::mutex> _(mutex_);

    result.error = HeraErrno::OK;
    result.reason = "";

    try {
        // Update member;
        profiles_ = profiles;

        log::debug << "Daemon::updateProfile index = " << profileIndex << log::endl;
        profile_index_ = profileIndex;

        // Call dump profiles
        dump_profiles();
    } catch (const std::exception& e) {
        log::error << e.what() << log::endl;
        log::error << "Daemon::Can not update profiles" << log::endl;
        result.error = HeraErrno::ErrorReadProfiles;
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz