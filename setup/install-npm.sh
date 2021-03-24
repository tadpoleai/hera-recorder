#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e
set -v

mkdir npm_install
cd npm_install

wget https://registry.npmjs.org/npm/-/npm-6.8.0.tgz
tar -xzf npm-6.8.0.tgz
cd package
node bin/npm-cli.js install -gf ../npm-6.8.0.tgz

cd ../..
rm -rf npm_install

npm config set registry https://registry.npm.taobao.org

