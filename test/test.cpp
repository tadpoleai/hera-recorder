
#include <ifaddrs.h>
#include <iostream>
#include <unistd.h>
#include <vector>

#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

using IpV4Address = uint32_t;

struct Interface {
    std::string name;
    IpV4Address addr;
    IpV4Address netmask;
    IpV4Address broadcast;
    std::vector<Interface> children;
};

class Network final {
public:
    Network()
    {
        refresh();
    }

    bool refresh();

private:
    std::vector<Interface> interface;
};

int main()
{
    struct ifaddrs* ifList;
    if (getifaddrs(&ifList) < 0) {
        std::cout << "Broadcaster: Error, can not get ifs" << std::endl;
    }

    for (auto ifa = ifList; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        if (ifa->ifa_flags && IFF_LOOPBACK) {
            continue;
        }

        bool find = false;
        std::string ifa_name(ifa->ifa_name);

        std::cout << "IFA_NAME: " << ifa_name << std::endl;
        std::cout << "ifa_addr: " << inet_ntoa(((struct sockaddr_in*)ifa->ifa_addr)->sin_addr) << std::endl;
        std::cout << "ifa_netmask: " << inet_ntoa(((struct sockaddr_in*)ifa->ifa_netmask)->sin_addr) << std::endl;
        std::cout << "ifa_netmask: " << inet_ntoa(((struct sockaddr_in*)ifa->ifa_ifu.ifu_dstaddr)->sin_addr)
                  << std::endl;
    }
    freeifaddrs(ifList);
}