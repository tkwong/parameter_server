#!/usr/bin/env bash
rm -rf build
mkdir -p build
pushd .
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
popd
