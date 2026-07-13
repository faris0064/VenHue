# VenHue

VenHue is a program that syncs your Phillips Hue lights to in-game venue lighting from Rock Band 3 Deluxe on RPCS3. At the moment, only charts with MIDI venues are supported, and effects applied to Phillips Hue lights are only static colors.

# Building

The EDK source is not included with the project due to licensing. Request the EDK from the [Hue developer program](https://developers.meethue.com/edk-access-request/) and clone the repo it at `libs/EDK`.

## Building on Linux with Nix

### Requirements
- Nix with flakes enabled. [Easiest way to install.](https://github.com/DeterminateSystems/nix-installer)

### 1. Enter the development shell

```sh
nix develop
```

### 2. Build the Philips Hue EDK
The EDK libraries must be built before the main project.

```sh
cmake -S libs/EDK -B libs/EDK/build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER="$PWD/scripts/edk-clang++" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TEST=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TOOLS=OFF \
  -DBUILD_SWIG=OFF \
  -DBUILD_WRAPPERS=OFF
cmake --build libs/EDK/build
```

### 3. Build and run VenHue

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
./build/linux-debug/appVenHue
```

## Building on Windows

### Requirements
- [Qt 6.8+](https://www.qt.io/development/download)
- [CMake 3.16+](https://cmake.org/download/)
- MSVC

### 1. Build the Philips Hue EDK

The EDK libraries must be built before the main project.

```powershell
cd libs\EDK
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

### 2. Build VenHue

Configure and build with CMake:

```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.8.x\msvc2022_64"
cmake --build build --config Release
```

The output will be at `build\Release\appVenHue.exe`.

# Usage

VenHue requires a custom build of Rock Band 3 Deluxe, which you can pull from the "develop" branch [here](https://github.com/faris0064/rock-band-3-deluxe). Before getting started, make sure that at least one Entertainment Area has been created via the Phillips Hue app. Upon launching the program, select RPCS3 and navigate to your Rock Band 3 USRDIR folder. Then, search for your Phillips Hue bridge and click connect. After that, you will need to press the sync button on your Phillips Hue bridge. Select an entertainment area, then you should be set up.
