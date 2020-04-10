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

if [[ $ARCH == aarch64 ]] || [[ $ARCH == arm* ]]; then
    echo "ARM-Crossplatform toolset activated"
else
    echo "ARM-Crossplatform toolset is not activated"
    echo "Please source <toolset_path>/environment-setup-aarch64-fsl-linux first"
    echo "Exiting"
    exit -1
fi

echo "Input install prefix for cross libhera(arm)"
install_path="/opt/s32v/hera"
read -e -i "$install_path" -p "Please input install prefix for libhera: " ans
install_path="${ans:-$install_path}"
echo

sudo mkdir -p $install_path/bin
sudo mkdir -p $install_path/lib
sudo mkdir -p $install_path/include

echo "Installing Libraries"
sudo chmod 755 lib/*
sudo mkdir -p ${install_path}/lib
sudo cp -r lib/lib*.so ${install_path}/lib
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
echo "set(HERA_INSTALL_PREFIX ${install_path})" >shared/FindHeraArm.cmake
cat shared/FindHera.cmake >>shared/FindHeraArm.cmake
echo "CMake module file is shared/FindHeraArm.cmake"
echo

read -p "Install CMake Package (y/N): " ans
if [[ $ans = [yY] ]]; then
    echo 'message("${CMAKE_ROOT}/Modules")' >tmp_cmake_root_cmake
    cmake_module_path=$(cmake -N -P tmp_cmake_root_cmake 2>&1)
    rm tmp_cmake_root_cmake

    read -e -i "$cmake_module_path" -p "Please confirm install prefix for CMake module: " ans
    cmake_module_path="${ans:-$cmake_module_path}"
    echo "Installating CMake Package"
    sudo cp shared/FindHeraArm.cmake $cmake_module_path
fi
echo

echo "Installation Completed"
