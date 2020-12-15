find_package(Qt5 COMPONENTS Widgets REQUIRED)

FILE(GLOB_RECURSE sources
  "*.cpp"
  "*.h"
)

ADD_LIBRARY(graph_qt STATIC ${sources})

TARGET_LINK_LIBRARIES(graph_qt
  base
  Qt5::Widgets
)

TARGET_INCLUDE_DIRECTORIES(graph_qt PUBLIC "..")
