# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Workflow

Do not commit changes automatically. Wait for explicit user request before
creating commits. Build and test locally before you do.

**There is no GitHub remote and no pull-request flow.** The only remote is a
local bare mirror — `/Users/alexsmn/tc/git/scada/graph_qt.git`, which is also
the URL the superproject's `.gitmodules` uses. Earlier guidance here said to
open a pull request instead of committing; there is nowhere to open one, so
review happens on the diff before the commit, not after it.

**The default branch is `add-vcpkg-manifest`, not `main`.** That is what
`origin/HEAD` points at and where the work has been landing; the name is a
leftover from how the branch started. `origin/main` is stale and abandoned —
it is fully contained in the default branch and has no unique commits — so do
not treat it as the mainline, and do not "fix" the situation by pushing there.
Land changes on the default branch when the user asks for a commit.

`.github/workflows/ci.yml` still triggers on `main` and is therefore dead: no
GitHub remote means it never runs, and it names a branch that is not the
default anyway. Do not rely on it as a gate — local `ctest` is the only check
that actually executes.

This repository is consumed as a submodule of the `scada` superproject, so a
change here is only half a change: the superproject needs a matching submodule
pointer bump before anything else sees it.

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
