# Graph Qt Widget Library

Qt5-based graphing widget library for displaying time-series data with interactive panning, zooming, and cursors.

## Prerequisites

- CMake 3.26+
- Visual Studio 2022 or later
- Qt 5.15.x (msvc2019 build)
- vcpkg with packages:
  - `gtest:x86-windows`
  - `icu:x86-windows`
- [ChromiumBase](https://github.com/user/chromebase) library

## Build

1. Copy `CMakeUserPresets.json.example` to `CMakeUserPresets.json` and update the paths for your system:
   - `CMAKE_TOOLCHAIN_FILE` - path to vcpkg toolchain
   - `CMAKE_MODULE_PATH` - path to ChromiumBase
   - `CMAKE_PREFIX_PATH` - path to Qt installation

2. Configure and build:

```batch
cmake --preset windows-x86-debug
cmake --build --preset windows-x86-debug
```

3. Run tests:

```batch
ctest --preset windows-x86-debug
```

## Outputs

- `graph_qt` - Static library
- `graph_qt_tester` - Demo application

## Updating Golden Screenshots

The rendering tests compare widget output against golden images stored in `testdata/`. To update golden screenshots after intentional visual changes:

1. Delete the outdated golden image(s) from `testdata/`
2. Run the tests - new golden images will be generated automatically
3. Verify the new images look correct
4. Commit the updated golden images

When a rendering test fails, it saves the actual output as `testdata/actual_*.png` for comparison.
