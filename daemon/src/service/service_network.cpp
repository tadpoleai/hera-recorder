
//
// Copyright 2018 Wayz.ai. All Rights Reserved.
//

#include "service.hpp"

namespace wayz {
namespace hera {
namespace daemon {

void Service::append_network_interfaces(std::vector<NetworkInterface>& _return, const std::vector<Interface>& input)
{
    for (const auto& base : input) {
        NetworkInterface ret_base;
        ret_base.ifName = base.name;
        ret_base.address = std::string(base.address);
        ret_base.netmask = std::string(base.netmask);
        ret_base.broadcast = std::string(base.broadcast);

        for (const auto& child : base.children) {
            SubInterface ret_child;
            ret_child.ifName = child.name;
            ret_child.address = std::string(child.address);
            ret_child.netmask = std::string(child.netmask);
            ret_child.broadcast = std::string(child.broadcast);

            ret_base.children.emplace_back(ret_child);
        }

        _return.emplace_back(ret_base);
    }
}

void Service::createNetworkInterface(std::vector<NetworkInterface>& _return,
                                     const std::string& ifBaseName,
                                     const std::string& address,
                                     const std::string& netmask)
{
    append_network_interfaces(_return, network_.create(ifBaseName, address, netmask));
}

void Service::retrieveNetworkInterface(std::vector<NetworkInterface>& _return)
{
    append_network_interfaces(_return, network_.retrieve());
}

void Service::updateNetworkInterface(std::vector<NetworkInterface>& _return,
                                     const std::string& ifChildName,
                                     const std::string& address,
                                     const std::string& netmask)
{
    append_network_interfaces(_return, network_.update(ifChildName, address, netmask));
}

void Service::deleteNetworkInterface(std::vector<NetworkInterface>& _return, const std::string& ifChildName)
{
    append_network_interfaces(_return, network_.drop(ifChildName));
}

}  // namespace daemon
}  // namespace hera
}  // namespace wayz