cmake_minimum_required(VERSION 3.26)

# The shared build-support kit (ADR 0011): at this product's root in an export,
# at the tree root in the monorepo. Before project(), so the machine config it
# reads can still set the vcpkg variables the toolchain consumes there.
set(_scada_dir "${CMAKE_CURRENT_SOURCE_DIR}")
while(NOT EXISTS "${_scada_dir}/build-support/ScadaProducts.cmake")
  get_filename_component(_scada_parent "${_scada_dir}" DIRECTORY)
  if(_scada_parent STREQUAL _scada_dir)
    message(FATAL_ERROR
      "build-support/ not found above ${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
  set(_scada_dir "${_scada_parent}")
endwhile()
include("${_scada_dir}/build-support/ScadaProducts.cmake")
include("${_scada_dir}/build-support/ScadaProductBase.cmake")
unset(_scada_dir)
unset(_scada_parent)

scada_product_prologue()

project(graph_qt)

# C++20, not the tree default of 23: passed to the base call so it holds when
# graph_qt is spliced into a C++23 consumer too.
scada_product_base(CXX_STANDARD 20)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

# The shared kit is on the include path so a test entry point can say
# `#include "scada_qt_offscreen_platform.h"`. It resolves in both layouts --
# SCADA_PRODUCT_SEARCH_ROOT is the tree root in the monorepo and this product
# root in an export, and `build-support/` sits beneath it in both -- and it is
# set here rather than by scada_qt_import_offscreen_platform_into_tests(),
# which returns early on a Qt with no offscreen plugin. Tying the include path
# to that guard would turn a missing plugin into a compile error in the file
# that exists to cope with one.
include_directories("${SCADA_BUILD_SUPPORT_DIR}")

enable_testing()
find_package(GTest REQUIRED)

add_subdirectory(model)

add_library(graph_qt STATIC
  graph.cpp
  graph.h
  graph_axis.cpp
  graph_axis.h
  graph_cursor.h
  graph_line.cpp
  graph_line.h
  graph_pane.cpp
  graph_pane.h
  graph_plot.cpp
  graph_plot.h
  graph_time_helper.h
  graph_widget.cpp
  graph_widget.h
  horizontal_scroll_bar_controller.cpp
  horizontal_scroll_bar_controller.h
  test/golden_image.h
  test/test_data_source.h
)

target_link_libraries(graph_qt
  Qt6::Widgets
  graph_qt_model
)

target_include_directories(graph_qt PUBLIC "..")

set_property(TARGET graph_qt PROPERTY AUTOMOC ON)

# Unit tests.
add_executable(graph_qt_unittests
  graph_rendering_unittest.cpp
  graph_unittest.cpp
  test/golden_image_unittest.cpp
  test/unittest_main.cpp
)
target_link_libraries(graph_qt_unittests PRIVATE GTest::gmock graph_qt)
add_test(NAME graph_qt_unittests COMMAND graph_qt_unittests
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_subdirectory(app)

# Last, because it walks the targets that exist by now: the test executables
# get the offscreen platform plugin linked in, so a static Qt can honour the
# headless default `test/unittest_main.cpp` applies on macOS and so
# `QT_QPA_PLATFORM=offscreen` works for a standalone ctest run. Neither was
# true until 2026-09-20 -- see the same comment in view_manager_qt, and
# backlog 808.
scada_qt_import_offscreen_platform_into_tests()
