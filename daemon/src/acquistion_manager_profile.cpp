//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "acquisition_manager.hpp"


namespace wayz {
namespace hera {
namespace daemon {

void AcquisitionManager::read_profiles()
{
    try {
        std::ifstream ifs;
        ifs.open(JsonFile_, std::ios::in);
        ifs >> json_instance_;
        ifs.close();
    } catch (const std::exception& e) {
        try {
            std::ofstream ofs;
            json_instance_ = json::parse("[]");
            ofs.open(JsonFile_, std::ios::out);
            ofs << json_instance_.dump(4);
            ofs.close();
        } catch (const std::exception& e) {
            log::error << e.what() << log::endl;
            log::error << "Acquistion::Can not read profiles" << log::endl;
        }
    }
}

void AcquisitionManager::getProfile(ProfileResult& result)
{
    log::info << "Acquisition::getProfile called" << log::endl;
    result.error = HeraErrno::OK;
    result.reason = "";
    try {
        result.profiles = json_instance_.dump();
    } catch (const std::exception& e) {
        log::error << e.what() << log::endl;
        log::error << "Acquistion::Can not get profiles" << log::endl;
        result.profiles = "[]";
        result.error = HeraErrno::ErrorReadProfiles;
    }
}

void AcquisitionManager::updateProfile(ProfileResult& result, const std::string& json_str)
{
    log::info << "Acquisition::updateProfile called" << log::endl;
    result.error = HeraErrno::OK;
    result.reason = "";
    try {
        json_instance_ = json::parse(json_str);

        log::debug << "Acquistion::updateProfile parsing succeed" << log::endl;

        try {
            std::ofstream ofs;
            ofs.open(JsonFile_, std::ios::out);
            ofs << json_instance_.dump(4);
            ofs.close();
        } catch (const std::exception& e) {
            log::error << e.what() << log::endl;
            log::error << "Acquistion::Can not write profiles" << log::endl;
        }
    } catch (const std::exception& e) {
        log::error << e.what() << log::endl;
        log::error << "Acquistion::Can not update profiles" << log::endl;
        result.error = HeraErrno::ErrorReadProfiles;
    }
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz