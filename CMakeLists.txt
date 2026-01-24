cmake_minimum_required(VERSION 3.26)

project(graph_qt)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

file(GLOB SOURCES CONFIGURE_DEPENDS
  "*.cpp"
  "*.h"
  "model/*.cpp"
  "model/*.h"
  "test/*.cpp"
  "test/*.h"
)

set(SOURCES_UT ${SOURCES})
list(FILTER SOURCES EXCLUDE REGEX  "unittest")
list(FILTER SOURCES_UT INCLUDE REGEX "unittest")

add_library(graph_qt STATIC ${SOURCES})

find_package(ChromiumBase REQUIRED)

target_link_libraries(graph_qt
  ChromiumBase::base
  Qt5::Widgets
)

target_include_directories(graph_qt PUBLIC "..")

set_property(TARGET graph_qt PROPERTY AUTOMOC ON)

# UTs

enable_testing()
find_package(GTest REQUIRED)

add_executable(graph_qt_unittests ${SOURCES_UT})
target_link_libraries(graph_qt_unittests PRIVATE GTest::gmock graph_qt)
# Register test executable with CTest. Individual gtest tests run within
# the executable and report results via gtest's own output.
add_test(NAME graph_qt_unittests COMMAND graph_qt_unittests)

add_subdirectory(app)
