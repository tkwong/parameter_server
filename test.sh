#!/usr/bin/env bash
NUM_CORE=getconf _NPROCESSORS_ONLN
mkdir -p build
pushd .
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug && make -j$NUM_CORE && GLOG_logtostderr=1 ./HuskyUnitTest
popd
