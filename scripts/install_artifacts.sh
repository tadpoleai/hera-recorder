#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

# This file is for deploying package built by jenkins-ci

cd $(dirname "$0")

# if [ ! -d "bin" ] || [ ! -d "header" ] || [ ! -d "lib" ] || [ ! -d "plugin" ] || [ -d ".git" ]; then
#     echo "This script is not designed to be executed directly in this repo"
#     exit 1
# fi

install_prefix="/usr/local/"
install_cross_compile=false
cross_compile_toolset_path="/opt/fsl-auto/2.5.2/"

install_carto=false
install_client=false
install_daemon=false

PrintUsage() {
    echo ""
    echo "This program installs Wayz-Hera"
    echo "usage: $0 [-p <prefix>] [-m <module> [-m <module> ...]] [-c <cross_compile_toolset_path>] [-h]"
    echo "  -p  Install Prefix of Hera Headers, Binraries and Libraries, <prefix> default = '/usr/local/'"
    echo "  -c  Install Cross Compile and use Toolset_path = <cross_compile_toolset_path>, only available when host is x86_64"
    echo "  -m  Install Specific Module, <module> = carto, client, daemon"
    echo "      carto   Install Carto"
    echo "      client  Install Client"
    echo "      daemon  Install Daemon"
    echo "  -h  Print Help Information"
    echo ""
    exit 1
}

while getopts "p:c:m:h" opt; do
    case "$opt" in
    p)
        install_prefix=$OPTARG
        ;;
    m)
        case "${OPTARG}" in
        carto)
            install_carto=true
            ;;
        client)
            install_client=true
            ;;
        daemon)
            install_daemon=true
            ;;
        esac
        ;;
    c)
        install_cross_compile=true
        cross_compile_toolset_path=$OPTARG
        ;;
    h)
        PrintUsage
        ;;
    \?)
        PrintUsage
        ;;
    esac
done

case "$(uname -i)" in
x86_64 | amd64)
    hostarch="amd64"
    ;;
i?86)
    echo "This program only supports x86_64(amd64) or arm(aarch64)s"
    ;;
arm*)
    hostarch="arm"
    ;;
powerpc | ppc64)
    echo "This program only supports x86_64(amd64) or arm(aarch64)s"
    ;;
esac

if [[ ${install_cross_compile} == true ]] && [[ $hostarch == "amd64" ]]; then
    unset LD_LIBRARY_PATH
    source ${cross_compile_toolset_path}/environment-setup-aarch64-fsl-linux
    arch="arm"
    echo "Installing Cross Compile on $hostarch targeting $arch"
else
    echo "Installing Target on $hostarch"
    arch=$hostarch
fi

# Install Headers
mkdir -p ${install_prefix}/include/hera
cp -r header/hera/* ${install_prefix}/include/hera

# Install Binraries and Libraries
chmod 755 bin/$arch/*
chmod 755 lib/$arch/*
chmod 755 plugin/$arch/base/*
chmod 755 plugin/$arch/driver/*
mkdir -p ${install_prefix}/bin
mkdir -p ${install_prefix}/lib
mkdir -p ${install_prefix}/lib/hera/plugin/base
mkdir -p ${install_prefix}/lib/hera/plugin/driver
cp -r bin/$arch/* ${install_prefix}/bin
cp -r lib/$arch/* ${install_prefix}/lib
cp -r plugin/$arch/base/* ${install_prefix}/lib/hera/plugin/base
cp -r plugin/$arch/driver/* ${install_prefix}/lib/hera/plugin/driver

# Install CMake Module
echo "set(HERA_INSTALL_PREFIX ${install_prefix})" >share/FindHera.cmake
cat share/FindHera.cmake.inc >>share/FindHera.cmake
echo 'message("${CMAKE_ROOT}/Modules")' >tmp_cmake_root_cmake
cmake_module_path=$(cmake -N -P tmp_cmake_root_cmake 2>&1)
rm tmp_cmake_root_cmake
cp share/FindHera.cmake $cmake_module_path

# Install Carto
if [[ ${install_carto} == true ]] && [[ $hostarch == "amd64" ]] && [[ $arch == "amd64" ]]; then
    chmod 755 share/carto/bin/*
    mkdir -p $install_path/share/hera/slam/carto
    cp -r share/carto/share $install_path/share/hera/slam/carto/
    cp share/carto/bin/* $install_path/bin
fi

# Install Client
if [[ ${install_client} == true ]] && [[ $hostarch == $arch ]]; then
    rm -rf /var/www/hera-client
    mkdir -p /var/www/hera-client
    cp -r client/* /var/www/hera-client
fi

# Install Daemon
if [[ ${install_daemon} == true ]] && [[ $hostarch == $arch ]]; then
    # Install hera-daemon's service
    cp script/daemon/hera-daemon.service /lib/systemd/system

    # Make directory for hera-daemon
    mkdir -p /var/hera/
    mkdir -p /var/hera/data
    mkdir -p /var/hera/logs

    # Copy Config json
    cp share/config/daemon.json /var/hera

    # Enable boot-up hera-daemon
    systemctl enable hera-daemon.service
fi
