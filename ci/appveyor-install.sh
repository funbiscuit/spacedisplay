#!/bin/bash
# Run from project root

echo Installing required dependencies
sudo add-apt-repository --yes ppa:ubuntu-sdk-team/ppa
sudo apt-get update -qq
sudo apt-get -y install qtbase5-dev libqt5svg5-dev qt5-default
mkdir -p tools && cd tools
# fetch linuxdeploy and qt plugin
wget -N https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
wget -N https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod a+x linuxdeploy-x86_64.AppImage linuxdeploy-plugin-qt-x86_64.AppImage
cd ..
