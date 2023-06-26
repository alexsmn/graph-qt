#include "graph_qt/graph.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_widget.h"

#include <QApplication>
#include <QScrollBar>
#include <QTimer>
#include <span>
#include <vector>

using namespace views;
using namespace std::chrono_literals;

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
  TestDataSource() {
    for (int i = 0; i < kInitialCount; ++i) {
      points_.emplace_back(kXOffset + i, i);
    }

    timer_.setInterval(1s);

    QObject::connect(&timer_, &QTimer::timeout, [this] {
      points_.emplace_back(kXOffset + points_.size(), points_.size());
      if (observer_) {
        observer_->OnDataSourceCurrentValueChanged();
        observer_->OnDataSourceHistoryChanged();
      }
    });

    timer_.start();
  }

  virtual double GetCurrentValue() const override { return points_.back().y; }

  virtual PointEnumerator* EnumPoints(double from,
                                      double to,
                                      bool include_left_bound,
                                      bool include_right_bound) override {
    return new TestPointEnumerator{points_};
  }

  virtual GraphRange GetHorizontalRange() const override {
    return {points_.front().x, points_.back().x};
  }

  static const int kInitialCount = 100;
  static const int kXOffset = 1000;

 private:
  std::vector<GraphPoint> points_;
  QTimer timer_;
};

int main(int argc, char** argv) {
  QApplication app{argc, argv};

  Graph graph;
  graph.setMinimumSize({1200, 800});

  auto* pane = new GraphPane{};
  graph.AddPane(*pane);

  auto* line = new GraphLine{};
  line->SetDataSource(new TestDataSource);
  pane->plot().AddLine(*line);

  graph.horizontal_axis().SetRange(
      {TestDataSource::kXOffset + TestDataSource::kInitialCount / 5,
       TestDataSource::kXOffset + TestDataSource::kInitialCount * 4 / 5});

  graph.SetHorizontalScrollBarVisible(true);

  graph.show();

  return QApplication::exec();
}