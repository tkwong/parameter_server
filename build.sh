#!/usr/bin/env bash
mkdir -p build
pushd .
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j8
popd
