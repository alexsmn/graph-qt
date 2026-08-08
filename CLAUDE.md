# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Workflow

Do not commit changes automatically. Wait for explicit user request before
creating commits. Build and test locally before you do.

**There are two remotes.** `origin` is a local bare mirror —
`/Users/alexsmn/tc/git/scada/graph_qt.git`, which is also the URL the
superproject's `.gitmodules` uses. `github` is the public GitHub repository
[alexsmn/graph-qt](https://github.com/alexsmn/graph-qt) (note the hyphen; the
directory name uses an underscore). Anything pushed to `github` is public.

Earlier guidance in this file claimed there was no GitHub remote and no
pull-request flow. That was wrong on both counts — the repo has carried pull
requests (commit `78a01d4` is "… (#1)"). In practice review has been happening
on the diff before the commit rather than in a PR; keep doing that unless the
user asks for a PR.

**The default branch is `main`, on both remotes.** Land changes there when the
user asks for a commit.

Until 2026-07-26 the default was `add-vcpkg-manifest` — a name left over from
how that branch started — while `main` sat stale behind it. That branch was
fast-forward merged into `main`, and `origin/HEAD` on the mirror was repointed,
so the two remotes and the local clone now agree. Older guidance in this file
said `main` was abandoned and must not be pushed to; that no longer applies.

There is no CI as of 2026-07-26: `.github/workflows/ci.yml` was deleted that
day. It did run on GitHub — 8 recorded runs, on `main` pushes and on pull
requests — but **every run failed**, each after roughly 1h40m, and it still
installed Qt 5.15.2 and `qtbase5-dev` long after the build moved to Qt6. Local
`ctest` is the only check that actually executes; run it before asking for a
commit. A replacement workflow would need a Qt6 toolchain and a build that
finishes in reasonable time.

This repository is consumed as a submodule of the `scada` superproject, so a
change here is only half a change: the superproject needs a matching submodule
pointer bump before anything else sees it.

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
