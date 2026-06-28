# VenHue

VenHue is a program that syncs your Phillips Hue lights to in-game venue lighting from Rock Band 3 Deluxe on RPCS3. At the moment, only charts with MIDI venues are supported.

## Requirements

- Windows 10/11
- [Qt 6.8+](https://www.qt.io/download)
- [CMake 3.16+](https://cmake.org/download/)
- Phillips Hue EDK 2.2.0
- MSVC

## Building on Windows

### 1. Build the Philips Hue EDK

The EDK libraries must be built before the main project. For licensing reasons, they are not included with the repo. Request access to the EDK [here](https://developers.meethue.com/edk-access-request/) and place the EDK into `libs\EDK-2.2.0`.

```powershell
cd libs\EDK-2.2.0
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

This produces the required `.lib` files under `libs\EDK-2.2.0\build\bin\` and `libs\EDK-2.2.0\build\external_install\lib\`.

### 2. Build VenHue

Configure and build with CMake:

```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.8.x\msvc2022_64"
cmake --build build --config Release
```

The output will be at `build\Release\appVenHue.exe`.

## Usage

VenHue requires a custom build of Rock Band 3 Deluxe, which you can pull from here. Before getting started, make sure that at least one Entertainment Area has been created via the Phillips Hue app. Upon launching the program, select RPCS3 and navigate to your Rock Band 3 USRDIR folder. Then, search for your Phillips Hue bridge and click connect. After that, you will need to press the sync button on your Phillips Hue bridge. Select an entertainment area, then you should be set up.