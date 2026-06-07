add_executable(graph_qt_tester WIN32)

find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

file(GLOB APP_SOURCES "*.cpp" "*.h")
target_sources(graph_qt_tester PRIVATE ${APP_SOURCES})

target_link_libraries(graph_qt_tester PRIVATE graph_qt)
if(WIN32 AND TARGET Qt6::WinMain)
  target_link_libraries(graph_qt_tester PRIVATE Qt6::WinMain)
endif()
