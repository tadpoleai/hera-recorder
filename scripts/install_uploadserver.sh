#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

cd $(dirname "$0")

p_username="hera-upload"
p_home_folder="/hera"
p_data_folder="data"
p_quiet=false

op_username_set=false
op_home_folder_set=false
op_data_folder_set=false

print_usage() {
    echo ""
    echo "This program create "
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