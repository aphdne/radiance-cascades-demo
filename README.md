# Radiance Cascades Demo

1. [Requirements](#requirements)
2. [Setup (macOS, Linux)](#setup-macos-linux)
3. [Setup (Windows)](#setup-windows-visual-studio)

2D lighting demo.

Maze image texture initially generated via [mazegenerator.net](https://www.mazegenerator.net/). 

## Requirements

- CMake 3.25
    - A C++11 (or higher) compiler
- Raylib 5.5 (provided via CMake if not installed system-wide)

This currently builds and runs on macOS, Arch Linux (on X11), and Windows. No planned support for Wayland :P

## Setup (macOS, Linux) 

If Raylib is not installed on your machine, CMake will download and build Raylib for you in the build directory.

```bash
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

If Raylib is not installed on your machine, CMake will download and build Raylib for you in the build directory.

Via command prompt:

```bash
mkdir build
cd build
cmake ..
```

This will generate a VS solution file (.sln) for you to use for compilation.

Make sure to run any resulting .exe from the project root directory.
