add_library(graph_qt_model STATIC
  graph_data_source.cpp
  graph_data_source.h
  graph_range.h
  graph_types.h
)

target_compile_features(graph_qt_model PUBLIC cxx_std_20)

target_include_directories(graph_qt_model PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")

target_link_libraries(graph_qt_model
  Qt5::Widgets
)

# Unit tests
add_executable(graph_qt_model_unittests
  graph_range_unittest.cpp
)
target_link_libraries(graph_qt_model_unittests PRIVATE GTest::gtest_main graph_qt_model)
add_test(NAME graph_qt_model_unittests COMMAND graph_qt_model_unittests)
