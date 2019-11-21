#!/bin/bash

echo "Enabling hera service"
sudo mkdir -p /var/hera/
sudo mkdir -p /var/hera/data
sudo mkdir -p /var/hera/logs

sudo systemctl enable hera-daemon.service