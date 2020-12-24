#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e
set -v

sudo apt-get update
sudo apt-get install -y nodejs npm
sudo npm config set registry https://registry.npm.taobao.org

sudo npm install n -g
sudo n stable
