#!/usr/bin/env sh
set -evx
env | sort

mkdir build || true
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
CTEST_OUTPUT_ON_FAILURE=1 GLOG_logtostderr=1 ./HuskyUnitTest