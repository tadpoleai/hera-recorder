#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

# This file is for deploying package built by jenkins-ci

if [ ! -d "client" ] || [ ! -d "convert" ] || [ ! -d "daemon" ] || [ -d ".git" ]; then
    echo "This script is not designed to be executed directly in this repo"
    exit 1
fi

# Install client (web dist)
sudo rm -rf /var/www/hera-client
sudo cp -r client/dist /var/www/hera-client

# Install hera-daemon
sudo cp daemon/hera-daemon /usr/local/bin

# Install hera-client
sudo cp convert/hera-convert /usr/local/bin

# Install hera-daemon's service
sudo cp daemon/script/hera-daemon /etc/init.d
sudo cp daemon/script/hera-daemon.service /lib/systemd/system

# Make directory for hera-daemon
sudo mkdir -p /var/hera/
sudo mkdir -p /var/hera/data
sudo mkdir -p /var/hera/logs

# Enable boot-up hera-daemon
sudo systemctl enable hera-daemon.service

echo "Installation Succeed"
