#include "graph_qt/graph.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_widget.h"

#include <QApplication>

using namespace views;

int main(int argc, char** argv) {
  QApplication app{argc, argv};

  Graph graph;
  GraphPane pane;
  pane.Init(graph);
  GraphWidget graph_widget{pane};
  graph_widget.show();

  return QApplication::exec();
}