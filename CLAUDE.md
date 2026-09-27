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

**Publication is by export, and graph_qt is published** — as
`github.com/alexsmn/graph-qt` (`tools/export/products.toml`, the `graph_qt`
product, `published = true`; checked 2026-09-27). The export sends this
directory plus the shared `build-support/`, `ports/` and root `.clang-format`.
This paragraph said it was unpublished, with a local destination, until that
date; the manifest had moved on without it.

Earlier guidance in this file described `origin` and `github` remotes local to
this directory, a public GitHub repository, and a default branch of its own.
That model predates the graft into the superproject; none of those remotes
exists in this checkout. The GitHub repository is written only by
`tools/export/export.py --publish`, never pushed to by hand.

graph_qt has no workflow of its own: `.github/workflows/ci.yml` was deleted on
2026-07-26. Its tests do run in CI, though — the Qt client's workflow checks
this product out as a sibling and runs its suite on Windows, Ubuntu and macOS,
which is how its Linux golden images were recorded (see below). The deleted
workflow ran 8 times on GitHub and **every run failed**, each after roughly
1h40m, still installing Qt 5.15.2 long after the build moved to Qt6. Run local
`ctest` before asking for a commit; the client's CI sees a change only after
the next export.

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
in `testdata/`. Read and write them through
`build-support/scada_qt_golden_image.h` (namespace `scada::qt_test`) — never
`QImage::save()` onto a golden path directly.

**The goldens are per platform**, because offscreen Qt draws text with the
host's fonts. The PNGs at the top of `testdata/` are the Windows set;
`testdata/linux/` is the Linux set, recorded on the Qt client's CI
`ubuntu-latest` runner (2026-09-27) and selected by `GetTestDataPath()`; macOS
compares against the Windows set and skips a mismatch. A Linux golden that is
missing is written by the test, which then skips, and that CI job uploads it
as the `linux-goldens` artifact to review and commit. The time axis is
rendered in local time, so the date labels — and the goldens — also depend on
the time zone the tests run in: CI runners are UTC.

**Deleting a golden is the only way to ask for a new baseline.** A golden that
is present but does not decode — truncated, corrupt, or a build without the PNG
codec — fails the test and tells you to restore it from git; it is never treated
as "no baseline yet". The two used to be one condition, and a run without the
codec zeroed two of the sibling `view_manager_qt` goldens on 2026-08-08 and then
rebaselined them from whatever it had just rendered. `QImageWriter` opens and
truncates its destination before it discovers it has no encoder, which is why
`scada::qt_test::SaveGoldenImage` encodes to a scratch sibling and moves it
into place only once it is whole.

**The helper is shared, not copied.** It lived here and in a byte-identical
copy under `view_manager_qt` — differing only in include guard and namespace,
and with a second copy of its 147-line test below it — until 2026-09-20, on the
reasoning that the two are separately exportable products under ADR 0011 and
neither may include from the other. True, and answered by a third place:
`build-support/`, the shared kit, which every export carries at its own root.
Reaching it costs one `include_directories("${SCADA_BUILD_SUPPORT_DIR}")` near
the top of `CMakeLists.txt`, and the test comes along with it — this product's
test executable names
`${SCADA_BUILD_SUPPORT_DIR}/scada_qt_golden_image_unittest.cpp` as a source, so
the helper is still verified in this product's own build. A safety fix kept in
two hand-synced copies is exactly the one you do not want drifting, and this
one exists because a drifted-from version destroyed two tracked baselines.

## CMake

Do not use `file(GLOB ...)` in CMakeLists.txt. List source files explicitly to ensure proper rebuild detection when files are added or removed.

### The tests run offscreen on macOS

`test/unittest_main.cpp` calls `scada::qt_test::DefaultToOffscreenPlatform()`
before it builds the `QApplication`, which on macOS sets `QT_QPA_PLATFORM` to
`offscreen:configfile=<a 1920x1080 screen>` unless the caller named a
platform. Without it, a `ctest` sweep bounced a Dock icon once per case.

Two preconditions, neither of which held until 2026-09-20:

- **The plugin has to be in the Qt**, which is a `vcpkg.json` question rather
  than a code one. qtbase gates `src/plugins/platforms/offscreen` on
  `QT_FEATURE_freetype`, so with `"default-features": false` and no
  `freetype` it is never built. `$direct` names it, beside Linux
  `fontconfig`, for that reason alone. Check with
  `QT_QPA_PLATFORM=nosuchplatform` on the test binary, which prints the list —
  it must say `cocoa, offscreen`.
- **The plugin has to be linked in**, because the Qt here is static.
  `scada_qt_import_offscreen_platform_into_tests()` at the end of
  `CMakeLists.txt` does that and passes the screen description's path as the
  `SCADA_QT_OFFSCREEN_PLATFORM_CONFIG` definition, the only thing the helper
  keys on.

The helper is `build-support/scada_qt_offscreen_platform.h` — shared rather
than copied, since this product may include from no other one — reached
through the `include_directories("${SCADA_BUILD_SUPPORT_DIR}")` near the top
of `CMakeLists.txt`.
