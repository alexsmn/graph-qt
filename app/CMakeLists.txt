add_executable(graph_qt_tester WIN32)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

file(GLOB APP_SOURCES "*.cpp" "*.h")
target_sources(graph_qt_tester PRIVATE ${APP_SOURCES})

target_link_libraries(graph_qt_tester PRIVATE graph_qt)
if(WIN32)
  target_link_libraries(graph_qt_tester PRIVATE Qt5::WinMain)
endif()
