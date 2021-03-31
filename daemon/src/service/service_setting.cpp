//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "service.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::load_setting()
{
    log::debug << "Daemon: Loading Setting" << log::endl;

    try {
        std::ifstream ifs;
        ifs.open(AcquisitionSettingFileName_, std::ios::in);
        json json_;
        ifs >> json_;
        acquisition_setting_ = json_;
        ifs.close();
    } catch (const std::exception& e) {
        try {
            dump_setting();
            log::info << "Daemon: Created empty setting";
        } catch (const std::exception& e) {
            log::error << e.what() << log::endl;
            log::error << "Daemon: Can not load setting" << log::endl;
        }
    }
}

void Service::dump_setting()
{
    log::debug << "Daemon: dump setting called" << log::endl;

    try {
        std::ofstream ofs;
        ofs.open(AcquisitionSettingFileName_, std::ios::out);
        json json_ = acquisition_setting_;
        ofs << json_.dump(2);
        ofs.close();
    } catch (const std::exception& e) {
        log::error << e.what() << log::endl;
        log::error << "Daemon: Can not dump setting" << log::endl;
    }
}

void Service::getSetting(AcquisitionSetting& result)
{
    result = acquisition_setting_;
}

void Service::setProfiles(AcquisitionSetting& result, const std::vector<Profile>& profiles)
{
    log::debug << "Daemon: setProfiles called" << log::endl;

    acquisition_setting_.profiles = profiles;

    dump_setting();

    result = acquisition_setting_;
}

void Service::selectProfile(AcquisitionSetting& result, const int32_t profileIndex)
{
    log::debug << "Daemon: selectProfile called" << log::endl;

    acquisition_setting_.profileIndex = profileIndex;

    dump_setting();

    result = acquisition_setting_;
}

void Service::setOperatorInfo(AcquisitionSetting& result, const OperatorInfo& operatorInfo)
{
    log::debug << "Daemon: setOperatorInfo called" << log::endl;

    acquisition_setting_.operatorInfo = operatorInfo;

    dump_setting();

    result = acquisition_setting_;
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz