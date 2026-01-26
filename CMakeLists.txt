cmake_minimum_required(VERSION 3.26)

project(graph_qt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

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
  Qt5::Widgets
  graph_qt_model
)

target_include_directories(graph_qt PUBLIC "..")

set_property(TARGET graph_qt PROPERTY AUTOMOC ON)

# UTs

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
