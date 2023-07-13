#include "graph_qt/graph.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_widget.h"
#include "graph_qt/test/test_data_source.h"

#include <QApplication>
#include <QMainWindow>
#include <QScrollBar>
#include <QToolBar>
#include <vector>

using namespace views;
using namespace std::chrono_literals;

int main(int argc, char** argv) {
  QApplication app{argc, argv};

  UpdatingTestDataSource data_source{1s};

  QMainWindow main_window;
  auto* toolbar = main_window.addToolBar("ToolBar");

  Graph* graph = new Graph{&main_window};

  auto* pane = new GraphPane{};
  graph->AddPane(*pane);

  pane->plot().AddLine(data_source);

  graph->horizontal_axis().SetRange(
      {TestDataSource::kXOffset + TestDataSource::kInitialCount / 5,
       TestDataSource::kXOffset + TestDataSource::kInitialCount * 4 / 5});

  graph->SetHorizontalScrollBarVisible(true);

  auto* fit_action = toolbar->addAction("Fit");
  fit_action->setCheckable(true);
  fit_action->setChecked(graph->horizontal_axis().time_fit());
  QObject::connect(fit_action, &QAction::toggled, [graph](bool on) {
    graph->horizontal_axis().SetTimeFit(on);
  });

  main_window.setCentralWidget(graph);
  main_window.setMinimumSize({1200, 800});
  main_window.show();

  return QApplication::exec();
}