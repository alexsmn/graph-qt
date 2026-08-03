#include "graph_qt/graph.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_time_helper.h"
#include "graph_qt/graph_widget.h"
#include "graph_qt/test/test_data_source.h"

#include <QApplication>
#include <QMainWindow>
#include <QScrollBar>
#include <QTimer>
#include <QToolBar>
#include <chrono>
#include <vector>

using namespace views;
using namespace std::chrono_literals;

class UpdatingTestDataSource : public VirtualDataSource {
 public:
  explicit UpdatingTestDataSource(std::chrono::milliseconds update_period)
      : VirtualDataSource{dataset_} {
    timer_.setInterval(update_period);

    QObject::connect(&timer_, &QTimer::timeout, [this] { AddPoint(); });

    timer_.start();
  }

 private:
  VirtualDataset dataset_{.horizontal_min_ = -1000,
                          .step_ = 100,
                          .count_ = 100000,
                          .ramp_count_ = 100};

  QTimer timer_;
};

class UpdatingTimeDataSource : public VirtualDataSource {
 public:
  explicit UpdatingTimeDataSource(std::chrono::milliseconds update_period)
      : VirtualDataSource{dataset_} {
    timer_.setInterval(update_period);

    QObject::connect(&timer_, &QTimer::timeout, [this] { AddPoint(); });

    timer_.start();
  }

 private:
  static constexpr auto kStep = std::chrono::seconds(1);
  static constexpr auto kRange = std::chrono::hours(365 * 24);

  VirtualDataset dataset_{
      .horizontal_min_ = ValueFromTime(QDateTime::currentDateTime().addSecs(
          -std::chrono::duration_cast<std::chrono::seconds>(kRange).count())),
      .step_ = ValueFromDuration(kStep),
      .count_ = static_cast<size_t>(kRange / kStep),
      .ramp_count_ = 100};

  QTimer timer_;
};

int main(int argc, char** argv) {
  QApplication app{argc, argv};

  UpdatingTimeDataSource data_source{1s};

  QMainWindow main_window;
  auto* toolbar = main_window.addToolBar("ToolBar");

  Graph* graph = new Graph{&main_window};

  auto* pane = new GraphPane;
  graph->AddPane(*pane);

  pane->plot().AddLine(data_source);

  graph->horizontal_axis().SetRange(data_source.GetHorizontalRange());

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