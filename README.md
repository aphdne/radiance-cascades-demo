# Radiance Cascades Demo

1. [Requirements](#requirements)
2. [Setup (macOS, Linux)](#setup-macos-linux)
3. [Setup (Windows)](#setup-windows-visual-studio)
3. [Controls](#controls)

2D lighting demo. Intended for 1080p displays.

Maze image texture initially generated via [mazegenerator.net](https://www.mazegenerator.net/).

## Requirements

- CMake 3.25
    - A C++11 (or higher) compiler
- Raylib 5.5 (provided via CMake if not installed system-wide)
    - If Raylib is not [installed](https://github.com/raysan5/raylib#build-and-installation) on your machine, CMake will download and build Raylib for you in the build directory.

This currently builds and runs on macOS, Arch Linux (on X11), and Windows. No planned support for Wayland :P

## Setup (macOS, Linux)

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

Via command prompt:

```bash
mkdir build
cd build
cmake ..
```

This will generate a VS solution file (.sln) for you to use for compilation.

Make sure to run any resulting .exe from the project root directory.

## Controls

1, 2, & 3 toggles between drawing, lighting, and viewing mode.

Scrolling up or down increase brush/light size depending on if you are in drawing or lighting mode.

C clears the canvas if in drawing mode, and removes all lights if in lighting mode.

R resets the canvas to the original maze layout if in drawing mode, and resets to the starting lights if in lighting mode.

F3 to open the debug UI. When the debug UI is open, scrolling the mouse will change the amount of shadow cascades.

Left-mouse-button erases in drawing mode, or deletes nearby lights when in lighting mode.

Right-mouse-button draws in drawing mode, or places a light when in lighting mode.

Middle-mouse-button can be used to move lights around in any mode.
