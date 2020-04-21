#!/bin/bash

mkdir -p cmake-build-release
cd cmake-build-release || exit
cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - MinGW Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND" ..
cmake --build . --target spacedisplay_gui
cd ..
mkdir -p bin
cp cmake-build-release/app-gui/spacedisplay_gui.exe bin/spacedisplay_gui.exe
