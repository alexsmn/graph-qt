# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Workflow

Do not commit changes automatically. Wait for explicit user request before
creating commits. Build and test locally before you do.

**In the `scada` tree there is one remote, and it is the superproject's.**
`third_party/graph_qt/` is a plain tracked directory of the one repository, not
a clone of its own: `git remote -v` from anywhere in the tree reports the
superproject's `origin`, and `git rev-parse --show-toplevel` returns the tree
root. So a commit here is an ordinary commit on the superproject's branch, and
review happens on the diff before the commit — there is no separate history to
push and no pull-request flow to use unless the user asks for one.

**Publication is by export, and graph_qt is not published.** The export manifest
(`tools/export/products.toml`, the `graph_qt` product) sends this directory plus
the shared `build-support/`, `ports/` and root `.clang-format` to
`/Users/alexsmn/tc/git/scada/graph_qt.git`, and marks it `published = false`;
`tools/export/export.py` refuses to publish an unpublished product without
`--allow-new-publication`. The five products marked `published = true` are
`core`, `common`, `client`, `scada-docs` and `opcuapp` — graph_qt is not among
them.

Earlier guidance in this file described `origin` and `github` remotes local to
this directory, a public GitHub repository, and a default branch of its own.
That model predates the graft into the superproject; none of those remotes
exists in this checkout. If a GitHub repository of that name still exists it is
outside the supported publication path, so do not push to it — ask the user.

There is no CI as of 2026-07-26: `.github/workflows/ci.yml` was deleted that
day. It did run on GitHub — 8 recorded runs, on `main` pushes and on pull
requests — but **every run failed**, each after roughly 1h40m, and it still
installed Qt 5.15.2 and `qtbase5-dev` long after the build moved to Qt6. Local
`ctest` is the only check that actually executes; run it before asking for a
commit. A replacement workflow would need a Qt6 toolchain and a build that
finishes in reasonable time.

A change here needs no pointer bump and no second step: `graph_qt` is a plain
tree in the `scada` superproject, so committing it *is* publishing it to
everything that consumes it. `.gitmodules` declares exactly one submodule in the
whole tree — `third_party/libiec61850` — and
`git ls-tree -r HEAD | awk '$2=="commit"'` returns that one gitlink. Earlier
guidance here said the superproject needed a matching submodule pointer bump;
there is no pointer to bump, so a session that believed it went hunting for a
`.gitmodules` entry that does not exist, or concluded a landed change had not
propagated when it already had.

## Build Commands

graph_qt builds standalone. Set `VCPKG_ROOT` in the environment; anything else
machine-specific goes in `.scada-local.cmake` beside `build-support/`
(ADR 0011). On Windows run from a Visual Studio Developer Command Prompt so the
Ninja generator finds `cl.exe`.

```bash
# Configure
cmake --preset ninja

# Build
cmake --build --preset release      # or: debug, relwithdebinfo

# Run unit tests
ctest --preset test-release         # or: test-debug

# Run the test application
build/ninja/bin/Release/graph_qt_tester
```

See [README.md](README.md) for prerequisites.

## Architecture

This is a Qt6-based graphing library in the `views` namespace. The widget hierarchy:

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

### Golden images

`graph_rendering_unittest.cpp` compares rendered output against the tracked PNGs
in `testdata/`. Read and write them through `test/golden_image.h` — never
`QImage::save()` onto a golden path directly.

**Deleting a golden is the only way to ask for a new baseline.** A golden that
is present but does not decode — truncated, corrupt, or a build without the PNG
codec — fails the test and tells you to restore it from git; it is never treated
as "no baseline yet". The two used to be one condition, and a run without the
codec zeroed two of the sibling `view_manager_qt` goldens on 2026-08-08 and then
rebaselined them from whatever it had just rendered. `QImageWriter` opens and
truncates its destination before it discovers it has no encoder, which is why
`test::SaveGoldenImage` encodes to a scratch sibling and moves it into place
only once it is whole.

The helper is duplicated in `view_manager_qt` rather than shared: the two are
separately exportable products under ADR 0011 and neither may include from the
other.

## CMake

Do not use `file(GLOB ...)` in CMakeLists.txt. List source files explicitly to ensure proper rebuild detection when files are added or removed.
