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
  test/unittest_main.cpp
)
target_link_libraries(graph_qt_unittests PRIVATE GTest::gmock graph_qt)
add_test(NAME graph_qt_unittests COMMAND graph_qt_unittests
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_subdirectory(app)
