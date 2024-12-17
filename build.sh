#!/bin/bash

if [ ! -d "./build" ]; then
  mkdir build
fi

pushd build
cmake ..
make
popd

while getopts "r" arg; do
  ./build/radiance_cascades
done
