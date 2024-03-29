#!/bin/bash
# Run from project root

echo Starting build script
mkdir -p bin
mkdir -p build
cd build || exit
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTS=true -DTESTS_COV=ON
make
make install DESTDIR=AppDir
mv AppDir/usr/bin/spacedisplay_gui AppDir/usr/bin/spacedisplay
../tools/linuxdeploy-x86_64.AppImage --appdir AppDir --plugin qt --output appimage
# Remove AppDir, not needed anymore
rm -rf AppDir
mv Space_Display*.AppImage ../bin/SpaceDisplay-x86_64.AppImage

cd ..
