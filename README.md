# Graph Qt Widget Library

Qt6-based graphing widget library for displaying time-series data with interactive panning, zooming, and cursors.

## Screenshots

![Basic Graph](testdata/basic_graph.png)
![Multiple Lines](testdata/multiple_lines.png)
![Multiple Panes](testdata/multiple_panes.png)

## Prerequisites

- CMake 3.26+
- Visual Studio 2022 or later
- Ninja (for Ninja presets)
- Qt 6 with the Widgets module. `CMakeLists.txt` asks only for
  `find_package(Qt6 COMPONENTS Widgets)` — no minimum minor is enforced, and
  the version in use depends on where Qt comes from:
  - vcpkg `qtbase` (see `vcpkg.json`); the committed `builtin-baseline`
    resolves to 6.9.1, and Windows builds use the msvc x86/x64 triplets
  - Homebrew `qt` (6.11.1 as of this writing) when the `scada` superproject
    builds this repo on macOS via its `macos-local-client` preset
- vcpkg (dependencies listed in `vcpkg.json`)

## Build

Qt needs no path: with the vcpkg toolchain, manifest mode builds the `qtbase`
dependency from `vcpkg.json` into the build tree. Point `CMAKE_PREFIX_PATH` at
an existing Qt 6 installation only if you want one instead — and note that the
official Qt 6 installer ships no 32-bit MSVC kit, so a 32-bit build must use
vcpkg.

```bash
# Configure
cmake --preset ninja

# Build
cmake --build --preset release      # or: debug, relwithdebinfo

# Run tests
ctest --preset test-release         # or: test-debug
```

Every product in the SCADA tree carries this same preset set (ADR 0011), so the
commands do not change from one to the next, and there is no longer a
per-architecture preset or a `CMakeUserPresets.json` to copy. Set `VCPKG_ROOT`
in the environment; everything else machine-specific — target triplet, the MSVC
toolchain paths, compiler launchers — goes in `.scada-local.cmake` beside
`build-support/`. Output lands in `build/ninja/bin/<config>/`.

On Windows, run from a Visual Studio Developer Command Prompt so `cl.exe` is on
`PATH`, or set the MSVC search paths in `.scada-local.cmake`; see
`build-support/README.md`.

## Static Analysis (clang-tidy)

The project includes a `.clang-tidy` configuration for static analysis. Use the clang-tidy bundled with Visual Studio.

### Running clang-tidy

Run clang-tidy on source files using the VS bundled LLVM (adjust edition: Community/Professional/Enterprise):

```batch
"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\bin\clang-tidy.exe" ^
  -p build-ninja-x86 graph_line.cpp
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
