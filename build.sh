mkdir -p build
pushd .
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
popd
