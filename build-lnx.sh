#!/bin/bash

mkdir -p cmake-build-release
cd cmake-build-release || exit
cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" ..
cmake --build . --target spacedisplay_gui
cd ..
mkdir -p bin
cp cmake-build-release/app-gui/spacedisplay_gui bin/spacedisplay_gui
