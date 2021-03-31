#!/bin/bash

#
# Copyright 2018 Wayz.ai. All Rights Reserved.
#

set -e
set -v

SYSTEM="Ubuntu18.04"
FLYCAPTURE_PREFIX="flycapture2-2.13.3.31-amd64"
FLYCAPTURE_PKG="flycapture2-2.13.3.31"-$SYSTEM\_amd64-pkg.tgz

if [ ! -f "./$FLYCAPTURE_PKG" ]; then
    if [ -f "/mnt/nfs/hdmap/software/$FLYCAPTURE_PKG" ]; then
        cp /mnt/nfs/hdmap/software/$FLYCAPTURE_PKG .
    else
        echo "$FLYCAPTURE_PKG doesn't exist!"
        exit 1
    fi
fi

# sudo apt-get update
# sudo apt-get install -y \
#    libraw1394-11 \
#    libusb-1.0-0 \
#    libgtkmm-2.4-1v5 \
#    libglademm-2.4-1v5 \
#    libgtkmm-2.4-dev \
#    libgtkglextmm-x11-1.2-dev \
#    libglademm-2.4-dev

tar -xvf $FLYCAPTURE_PKG
pushd $FLYCAPTURE_PREFIX > /dev/null

#####################################
# The following code is copy from flycapture offical 'install_flycapture.h',
# but remove the confirm in it.
#####################################

sudo dpkg -i libflycapture-2*
sudo dpkg -i libflycapturegui-2*
sudo dpkg -i libflycapturevideo-2*
sudo dpkg -i libflycapture-c-2*
sudo dpkg -i libflycapturegui-c-2*
sudo dpkg -i libflycapturevideo-c-2*
sudo dpkg -i libmultisync-2*
sudo dpkg -i libmultisync-c-2*
sudo dpkg -i flycap-2*
sudo dpkg -i flycapture-doc-2*
sudo dpkg -i updatorgui*

if [ -z "$USER" ]; then
    echo "\$USER is empty, skip add a udev entry to allow access to 1394 and USB devices"
    exit 0
fi

grpname="flirimaging"
sudo groupadd -f $grpname
sudo usermod -a -G $grpname $USER

UdevFile="/etc/udev/rules.d/40-flir.rules";
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"2000\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"2001\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"2002\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"2003\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"2004\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"2005\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3000\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3001\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3004\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3005\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3006\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3007\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3008\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"300A\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"300B\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3100\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3101\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3102\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3103\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3104\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3105\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3106\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3107\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3108\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3109\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "ATTRS{idVendor}==\"1e10\", ATTRS{idProduct}==\"3300\", MODE=\"0664\", GROUP=\"$grpname\"" |\
    sudo tee -a $UdevFile
echo "KERNEL==\"raw1394\", MODE=\"0664\", GROUP=\"$grpname\"" | sudo tee -a $UdevFile
echo "KERNEL==\"video1394*\", MODE=\"0664\", GROUP=\"$grpname\"" | sudo tee -a $UdevFile
echo "SUBSYSTEM==\"firewire\", GROUP=\"$grpname\"" | sudo tee -a $UdevFile
echo "SUBSYSTEM==\"usb\", GROUP=\"$grpname\"" | sudo tee -a $UdevFile

sudo /etc/init.d/udev restart


echo
echo "Configuration complete. A reboot may be required on some systems for changes to take effect";
echo

rm -rf flycapture*

exit 0