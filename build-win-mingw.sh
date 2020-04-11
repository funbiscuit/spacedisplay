#!/bin/bash

mkdir -p cmake-build-release
cd cmake-build-release || exit
cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - MinGW Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND" ..
cmake --build . --target spacedisplay -- -j 3
cd ..
mkdir -p bin
cp cmake-build-release/spacedisplay.exe bin/spacedisplay.exe
