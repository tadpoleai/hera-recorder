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

is_arm=0
arch=$(uname -i)
if [[ $arch == aarch64 ]] || [[ $arch == arm* ]]; then
    echo "Target arch is ARM, Continue Installing"
else
    echo "Target arch is not ARM, Installing Aborted"
    exit -1
fi

echo "Input install prefix for libhera(arm)"
install_path="/usr/local"
read -e -i "$install_path" -p "Please input install prefix for libhera: " ans
install_path="${ans:-$install_path}"
mkdir -p $install_path
echo

echo "Installing Binraries"
chmod 755 bin/*
cp -r bin/hera-* $install_path/bin/
echo "Binraries installed to ${install_path}/bin"
echo

echo "Installing Libraries"
chmod 755 lib/*
cp -r lib/libhera-*.so $install_path/lib/
echo "Libraries installed to ${install_path}/lib"
echo
ldconfig

echo "Installing Headers"
mkdir -p $install_path/include/hera
cp -r include/* $install_path/include/hera
echo "Headers installed to ${install_path}/include/hera"

echo
echo "Installation Completed"
