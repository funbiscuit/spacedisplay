#!/bin/bash
# Run from project root

mkdir -p build_cov
cd build_cov || exit
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=true -DTESTS_COV=ON

cmake --build . --target coverage_xml
cmake --build . --target coverage_html
ls

cd ..
