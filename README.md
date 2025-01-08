# Radiance Cascades Demo

1. [Requirements](#requirements)
2. [Setup (macOS, Linux)](#setup-macos-linux)
3. [Setup (Windows)](#setup-windows-visual-studio)

2D lighting demo.

Maze image texture initially generated via [mazegenerator.net](https://www.mazegenerator.net/).

## Requirements

- CMake 3.25
    - A C++11 (or higher) compiler

This currently builds and runs on macOS Sequoia, Arch Linux (on X11), and Windows 11.

## Setup (macOS, Linux)

After cloning the repo and cd'ing into the repo folder:

```bash
# grab our libraries
git submodule update --init

# run the convenience build script
./build.sh    # to build without running
./build.sh -r # to build and run the resulting binary

# or build it manually via CMake
mkdir build
cd build
cmake ..
make
cd .. # run the resulting binary in the source directory, not the build directory
./radiance_cascades
```

## Setup (Windows Visual Studio)

Clone the repository and run the following from a command prompt within the repository's directory:

```bash
# grab our libraries
git submodule update --init

# build via CMake
mkdir build
cd build
cmake ..
```

This will generate a Visual Studio solution file (.sln) for you to use for compilation. Open this in Visual Studio and build it.

You may get an "access is denied" error. This can be ignored. The resulting executable will be in `build/Debug`.

Either **run from or move the executable to the project root directory so that it can access the resources folder**.
