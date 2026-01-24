# Graph Qt Widget Library

Qt5-based graphing widget library for displaying time-series data with interactive panning, zooming, and cursors.

## Screenshots

![Basic Graph](testdata/basic_graph.png)
![Multiple Lines](testdata/multiple_lines.png)
![Multiple Panes](testdata/multiple_panes.png)

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

## Static Analysis (clang-tidy)

The project includes a `.clang-tidy` configuration for static analysis.

**Requirements:** LLVM/Clang toolchain with `clang-tidy` ([LLVM releases](https://releases.llvm.org/)) in PATH.

### Running clang-tidy

**Option 1: CMake integration** (runs during build)

```batch
cmake --preset windows-x86-debug -DCLANG_TIDY=ON
cmake --build --preset windows-x86-debug
```

**Option 2: Direct invocation** (requires compile_commands.json)

```bash
# Generate compilation database (use Ninja generator)
cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ...

# Run on all source files
run-clang-tidy -p build -header-filter='graph_qt/.*'

# Run on a single file
clang-tidy -p build graph_line.cpp
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
