#!/bin/bash

mkdir -p cmake-build-release
cd cmake-build-release || exit
cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - Unix Makefiles" ..
cmake --build . --target spacedisplay
cd ..
mkdir -p bin
cp cmake-build-release/spacedisplay bin/spacedisplay
