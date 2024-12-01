#!/bin/bash

pushd build
cmake ..
make
popd
./build/radiance_cascades
