#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

# This file is for deploying package built by jenkins-ci

cd $(dirname "$0")

if [ ! -d "bin" ] || [ ! -d "lib" ] || [ ! -d "include" ] || [ -d ".git" ]; then
    echo "This script is not designed to be executed directly in this repo"
    exit 1
fi

# echo "Input install prefix for libhera(arm)"
install_path="/usr/local"
# read -e -i "$install_path" -p "Please input install prefix for libhera: " ans
# install_path="${ans:-$install_path}"
# mkdir -p $install_path
# echo
sudo mkdir -p $install_path/bin
sudo mkdir -p $install_path/lib
sudo mkdir -p $install_path/include

echo "Installing Binraries"
sudo chmod 755 bin/*
sudo cp -r bin/hera-* $install_path/bin/

echo "Installing Libraries"
sudo chmod 755 lib/*
sudo cp -r lib/lib*.so $install_path/lib/
sudo ldconfig

echo "Installing Headers"
sudo mkdir -p $install_path/include/hera
sudo cp -r include/* $install_path/include/hera

read -p "Install CMake Package (y/N): " ans
if [[ $ans = [yY] ]]; then
    cmake_module_path=$(echo 'message("${CMAKE_ROOT}")' >tmp_cmake_root && cmake -N -P tmp_cmake_root && rm tmp_cmake_root)/Modules
    read -e -i "$cmake_module_path" -p "Please confirm install prefix for CMake module: " ans
    cmake_module_path="${ans:-$cmake_module_path}"
    echo "Installating CMake Package"
    sudo cp shared/FindHera.cmake $cmake_module_path
fi
echo

echo
read -p "Install shared/carto (y/N): " ans
echo
if [[ $ans = [yY] ]]; then
    echo "Installating shared/carto"

    sudo chmod 755 shared/carto/bin/*
    sudo mkdir -p $install_path/share/hera/slam/carto
    sudo cp -r shared/carto/share $install_path/share/hera/slam/carto/
    sudo cp shared/carto/bin/* $install_path/bin
fi

echo
read -p "Install client (y/N): " ans
echo
if [[ $ans = [yY] ]]; then
    echo "Installating client"

    # Install client (web dist)
    sudo rm -rf /var/www/hera-client
    sudo mkdir -p /var/www/hera-client
    sudo cp -r client /var/www/hera-client
fi

echo
read -p "Install daemon (y/N): " ans
echo
if [[ $ans = [yY] ]]; then
    echo "Installating daemon"

    # Install hera-daemon's service
    sudo cp script/daemon/hera-daemon.service /lib/systemd/system

    # Make directory for hera-daemon
    sudo mkdir -p /var/hera/
    sudo mkdir -p /var/hera/data
    sudo mkdir -p /var/hera/logs

    # Enable boot-up hera-daemon
    sudo systemctl enable hera-daemon.service
fi

echo
echo "Installation Completed"
