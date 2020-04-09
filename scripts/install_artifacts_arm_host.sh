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

echo "This script is for installing cross libhera(arm) to a host linux(amd64) machine"

echo "Input install prefix for cross libhera(arm)"
install_path="/opt/s32v/hera"
read -e -i "$install_path" -p "Please input install prefix for libhera: " ans
install_path="${ans:-$install_path}"
echo

echo "Installing Libraries"
sudo chmod 755 lib/*
sudo mkdir -p ${install_path}/lib
sudo cp -r lib/libhera-*.so ${install_path}/lib
echo "Libraries installed to ${install_path}/lib"
ls ${install_path}/lib
echo

echo "Installing Headers"
sudo mkdir -p ${install_path}/include/hera
sudo cp -r include/* ${install_path}/include/hera
echo "Headers installed to ${install_path}/include/hera"
ls ${install_path}/include/hera
echo

echo "Generating cmake template"
echo "set(HERA_INSTALL_PREFIX ${install_path})" > shared/FindHeraArm.cmake
cat shared/FindHera.cmake >> shared/FindHeraArm.cmake
echo "CMake module file is shared/FindHeraArm.cmake"
echo

read -p "Install CMake Package (y/N): " ans
if [[ $ans = [yY] ]]; then
    install_path=/usr/share/$(ls /usr/share/ | grep 'cmake-' | head -1)/Modules
    read -e -i "$install_path" -p "Please confirm install prefix for CMake module: " ans
    install_path="${ans:-$install_path}"
    echo "Installating CMake Package"
    sudo cp shared/FindHera.cmake $install_path
fi
echo

echo "Installation Completed"
