cmake_minimum_required(VERSION 3.26)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

file(GLOB SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
  "model/*.cpp"
  "model/*.h"
)

add_library(graph_qt STATIC ${SOURCES})

find_package(ChromiumBase REQUIRED)

target_link_libraries(graph_qt
  ChromiumBase::base
  Qt5::Widgets
)

target_include_directories(graph_qt PUBLIC "..")

set_property(TARGET graph_qt PROPERTY AUTOMOC ON)

add_subdirectory(app)
