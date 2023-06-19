add_executable(graph_app WIN32)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

file(GLOB APP_SOURCES "*.cpp" "*.h")
target_sources(graph_app PRIVATE ${APP_SOURCES})

target_link_libraries(graph_app PRIVATE
  graph_qt
  Qt5::WinMain
)
