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

echo "Installing Binraries"
sudo chmod 755 bin/*
sudo cp -r bin/hera-* /usr/local/bin/

echo "Installing Libraries"
sudo chmod 755 lib/*
sudo cp -r lib/libhera-*.so /usr/local/lib/
sudo ldconfig

echo "Installing Headers"
sudo mkdir -p /usr/local/lib/hera
sudo cp -r include/* /usr/local/lib/hera
sudo cp shared/FindHera.cmake /usr/share/$(ls /usr/share/ | grep 'cmake-' | head -1)/Modules/

echo
read -p "Install shared/carto (y/N): " ans
echo
if [[ $ans = [yY] ]]; then
    echo "Installating shared/carto"

    sudo chmod 755 shared/carto/bin/*
    sudo mkdir -p /usr/local/share/hera/slam/carto
    sudo cp -r shared/carto/share /usr/local/share/hera/slam/carto/
    sudo cp shared/carto/bin/* /usr/local/bin
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
