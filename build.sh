#!/bin/bash

build() {
  if [ ! -d "./build" ]; then
    mkdir build
  fi

  pushd build
    rm ./radiance_cascades
    cmake ..
    make
  popd
}

while getopts ":rRc" arg; do
  case ${arg} in
    r)
      echo "build.sh :: building & running"
      build
      ./build/radiance_cascades
      exit
      ;;
    R)
      echo "build.sh :: running w/o building"
      ./build/radiance_cascades
      exit
      ;;
    c)
      echo "build.sh :: cleaning"
      rm -rf ./build/
      exit
      ;;
  esac
done

echo "build.sh :: building"
build
