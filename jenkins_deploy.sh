#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

# This file is for deploying package built by jenkins-ci

if [ ! -d "client" ] || [ ! -d "convert" ] || [ ! -d "daemon" ] || [ -d ".git" ]; then
    echo "This script is not designed to be executed directly in this repo"
    exit 1
fi

echo
read -n1 -p "Install convert (y/N): " ans
echo
if [[ $ans = [yY] ]]; then
    echo "Installating convert"

    # Install hera-daemon
    sudo cp convert/hera-convert /usr/local/bin
fi

echo
read -n1 -p "Install replay (y/N): " ans
echo
if [[ $ans = [yY] ]]; then
    echo "Installating replay"

    # Install hera-replay
    sudo cp replay/hera-replay /usr/local/bin
fi

echo
read -n1 -p "Install slam (y/N): " ans
echo
if [[ $ans = [yY] ]]; then
    echo "Installating slam"

    # Make directory for hera-slam
    sudo mkdir -p /usr/local/share/hera/slam/carto

    # Install hera-slam
    sudo cp -r slam/carto/share /usr/local/share/hera/slam/carto/
    sudo chmod 755 slam/carto/bin/*
    sudo cp slam/carto/bin/* /usr/local/bin
    sudo cp slam/bridge/hera-slam-bridge /usr/local/bin
    sudo cp slam/caller/hera-slam-caller-start /usr/local/bin
    sudo cp slam/caller/hera-slam-caller-stop /usr/local/bin
    sudo cp slam/result/hera-slam-result-test /usr/local/bin
fi

echo
read -n1 -p "Install client (y/N): " ans
echo
if [[ $ans = [yY] ]]; then
    echo "Installating client"

    # Install client (web dist)
    sudo rm -rf /var/www/hera-client
    sudo mkdir -p /var/www/hera-client
    sudo cp -r client/dist /var/www/hera-client
fi

echo
read -n1 -p "Install daemon (y/N): " ans
echo
if [[ $ans = [yY] ]]; then
    echo "Installating daemon"

    # Install hera-daemon
    sudo cp daemon/hera-daemon /usr/local/bin

    # Install hera-daemon's service
    sudo cp daemon/script/hera-daemon.service /lib/systemd/system

    # Make directory for hera-daemon
    sudo mkdir -p /var/hera/
    sudo mkdir -p /var/hera/data
    sudo mkdir -p /var/hera/logs

    # Enable boot-up hera-daemon
    sudo systemctl enable hera-daemon.service
fi

echo
echo "Installation Completed"
