#!/bin/bash

if [ ! -d "./build" ]; then
  mkdir build
fi

pushd build
cmake ..
make
popd

while getopts ":r" arg; do
  case ${arg} in
    r)
      ./build/radiance_cascades
      ;;
  esac
done
