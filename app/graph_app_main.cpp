#include "graph_qt/graph.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_widget.h"

#include <QApplication>
#include <QScrollBar>
#include <span>
#include <vector>

using namespace views;

class TestPointEnumerator : public PointEnumerator {
 public:
  explicit TestPointEnumerator(std::span<const GraphPoint> points)
      : points_{points} {}

  virtual size_t GetCount() const override { return points_.size(); }

  virtual bool EnumNext(GraphPoint& value) override {
    if (points_.empty()) {
      return false;
    }

    value = points_[0];
    points_ = points_.subspan(1);
    return true;
  }

 private:
  std::span<const GraphPoint> points_;
};

class TestDataSource : public GraphDataSource {
 public:
  explicit TestDataSource(std::vector<GraphPoint> points)
      : points_{std::move(points)} {
    
}

  virtual PointEnumerator* EnumPoints(double from,
                                      double to,
                                      bool include_left_bound,
                                      bool include_right_bound) override {
    return new TestPointEnumerator{points_};
  }

 private:
  const std::vector<GraphPoint> points_;
};

int main(int argc, char** argv) {
  QApplication app{argc, argv};

  Graph graph;
  graph.setMinimumSize({1200, 800});

  auto* pane = new GraphPane{};
  graph.AddPane(*pane);

  std::vector<GraphPoint> points;
  for (int i = 0; i < 1000; i += 10) {
    points.emplace_back(i, i);
  }

  auto* data_source = new TestDataSource{std::move(points)};

  auto* line = new GraphLine{};
  line->SetDataSource(data_source);
  pane->plot().AddLine(*line);

  graph.horizontal_axis().SetRange({300, 400});

  graph.horizontal_scroll_bar().setVisible(true);
  graph.horizontal_scroll_bar().setRange(0, 1000);

  graph.show();

  return QApplication::exec();
}