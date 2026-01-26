# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Workflow

Do not commit changes automatically. Wait for explicit user request before creating commits.

Do not commit directly to main. Create GitHub pull requests instead.

## Build Commands

```batch
# Configure (requires CMakeUserPresets.json with local paths)
cmake --preset windows-x86-debug

# Build
cmake --build --preset windows-x86-debug

# Run unit tests
ctest --preset windows-x86-debug

# Run the test application
build-windows\Debug\graph_qt_tester.exe
```

See [README.md](README.md) for prerequisites and `CMakeUserPresets.json` setup.

## Architecture

This is a Qt5-based graphing library in the `views` namespace. The widget hierarchy:

```
Graph (QFrame)
├── GraphAxis (horizontal, shared by all panes)
├── QSplitter
│   └── GraphPane* (multiple panes)
│       ├── GraphAxis (vertical, per-pane)
│       └── GraphPlot
│           └── GraphLine* (multiple lines per plot)
└── HorizontalScrollBarController
```

**Core Classes:**
- `Graph` - Main container widget. Manages panes, horizontal axis, cursors, and zooming history
- `GraphPane` - Contains a vertical axis and a plot area. Multiple panes can be stacked vertically
- `GraphPlot` - Handles rendering of lines and grid, mouse interaction (panning, zooming, cursor selection)
- `GraphLine` - Renders a single data series. Supports stepped, smooth, auto-range, and dots rendering modes
- `GraphAxis` - Renders axis ticks, labels, and cursor labels. Handles panning via mouse drag

**Model Layer** (`model/`):
- `GraphDataSource` - Abstract base for data providers. Implement `EnumPoints()` to supply data
- `GraphRange` - Value range with `low`/`high` bounds. Supports LINEAR, LOGICAL, and TIME kinds
- `GraphPoint` - Data point with `x`, `y` coordinates and `good` flag

**Data Flow:**
1. Create a `GraphDataSource` subclass to provide data points
2. Add a `GraphPane` to the `Graph`
3. Add a `GraphLine` to the pane's `GraphPlot` and connect it to your data source
4. The data source notifies observers (`OnDataSourceHistoryChanged`, etc.) when data updates

## Code Style

Uses Chromium C++ style (see `.clang-format`). Include order: local headers first, then system/third-party.

## Testing

Test files must be located next to the files where the tested functionality is defined. Name test files with a `_unittest.cpp` suffix (e.g., `graph_range.h` → `graph_range_unittest.cpp`).

## CMake

Do not use `file(GLOB ...)` in CMakeLists.txt. List source files explicitly to ensure proper rebuild detection when files are added or removed.
