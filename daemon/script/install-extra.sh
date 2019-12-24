#!/bin/bash

set -e

echo "Making directories for hera-daemon at /var/hera"
sudo mkdir -p /var/hera/
sudo mkdir -p /var/hera/data
sudo mkdir -p /var/hera/logs

echo "Enabling hera-daemon.service"
sudo systemctl enable hera-daemon.service
