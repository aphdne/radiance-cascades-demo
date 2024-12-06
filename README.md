# Radiance Cascades Demo

1. [Requirements](#requirements)
2. [Setup (macOS & Linux)](#setup-(macOS-&-Linux))
3. [Setup (Windows)](#setup-(windows))

Lorem ipsum (/ˌlɔː.rəm ˈɪp.səm/ LOR-əm IP-səm) is a dummy or placeholder text commonly used in graphic design, publishing, and web development to fill empty spaces in a layout that do not yet have content.

## Requirements

- CMake
    - A C++11 (or higher) compiler
- Raylib 

## Setup (macOS & Linux)

Install Raylib ([Linux](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux), [macOS](https://github.com/raysan5/raylib/wiki/Working-on-macOS)):

macOS:
```bash
brew install raylib
```

Arch Linux:
```bash
sudo pacman -S raylib
```

Compile via CMake:

```bash
mkdir build
cd build
cmake ..            # create makefile via CMake
make                # run makefile
./radiance_cascades # run the programme
```

## Setup (Windows)

TBD
