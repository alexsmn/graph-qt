#include "graph_qt/graph.h"

#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_time_helper.h"
#include "test/test_data_source.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <chrono>

namespace views {
namespace {

// Test data source with time-based x values and linear y values.
class TimeTestDataSource : public GraphDataSource {
 public:
  // Creates a data source with time-based data: y = slope * index + offset.
  TimeTestDataSource(QDateTime base_time,
                     std::chrono::seconds time_step,
                     int count,
                     double slope,
                     double y_offset = 0.0) {
    double time_value = ValueFromTime(base_time);
    double step_seconds = ValueFromDuration(time_step);
    for (int i = 0; i < count; ++i) {
      points_.emplace_back(time_value + i * step_seconds, slope * i + y_offset);
    }
  }

  double GetCurrentValue() const override { return points_.back().y; }

  std::unique_ptr<PointEnumerator> EnumPoints(double from,
                                              double to,
                                              bool include_left_bound,
                                              bool include_right_bound) override {
    return std::make_unique<TestPointEnumerator>(points_);
  }

  GraphRange GetHorizontalRange() const override {
    GraphRange range{points_.front().x, points_.back().x};
    range.set_time();
    return range;
  }

 private:
  std::vector<GraphPoint> points_;
};

// Returns the path to the testdata directory.
QString GetTestDataPath() {
  // Look for testdata relative to the source directory.
  QDir dir(QCoreApplication::applicationDirPath());
  // Navigate up from build directory to source.
  while (!dir.exists("testdata") && dir.cdUp()) {
  }
  return dir.filePath("testdata");
}

// Renders a widget to a QImage.
QImage RenderWidget(QWidget& widget) {
  widget.setFixedSize(400, 300);
  widget.show();
  // Process events to ensure layout is complete.
  QCoreApplication::processEvents();
  return widget.grab().toImage();
}

// Compares two images pixel-by-pixel.
// Returns the number of differing pixels.
int CompareImages(const QImage& actual, const QImage& expected) {
  if (actual.size() != expected.size()) {
    return -1;  // Size mismatch
  }

  int diff_count = 0;
  for (int y = 0; y < actual.height(); ++y) {
    for (int x = 0; x < actual.width(); ++x) {
      if (actual.pixel(x, y) != expected.pixel(x, y)) {
        ++diff_count;
      }
    }
  }
  return diff_count;
}

class GraphRenderingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    testdata_path_ = GetTestDataPath();
  }

  // Loads a golden image from testdata.
  // Returns null image if the file doesn't exist (expected on first run).
  QImage LoadGoldenImage(const QString& name) {
    QString path = testdata_path_ + "/" + name;
    return QImage(path);
  }

  // Saves an image to testdata (for generating golden images).
  void SaveGoldenImage(const QImage& image, const QString& name) {
    QString path = testdata_path_ + "/" + name;
    QDir().mkpath(testdata_path_);
    if (!image.save(path)) {
      ADD_FAILURE() << "Failed to save golden image: " << path.toStdString();
    }
  }

  // Saves actual output for debugging when test fails.
  void SaveActualImage(const QImage& image, const QString& name) {
    QString path = testdata_path_ + "/actual_" + name;
    image.save(path);
  }

  QString testdata_path_;
  TestDataSource data_source_;
};

TEST_F(GraphRenderingTest, BasicGraph) {
  Graph graph;
  graph.setFixedSize(400, 300);

  auto* pane = graph.AddPane();
  auto* line = pane->plot().AddLine(data_source_);
  line->SetColor(Qt::blue);

  // Set a fixed range for reproducible rendering.
  auto data_range = data_source_.GetHorizontalRange();
  graph.horizontal_axis().SetTimeFit(false);
  graph.horizontal_axis().SetRange(data_range);

  QImage actual = RenderWidget(graph);

  const QString golden_name = "basic_graph.png";
  QImage expected = LoadGoldenImage(golden_name);

  if (expected.isNull()) {
    // Golden image doesn't exist yet - save current output as golden.
    SaveGoldenImage(actual, golden_name);
    GTEST_SKIP() << "Golden image created. Re-run test to verify.";
  }

  int diff_pixels = CompareImages(actual, expected);

  if (diff_pixels != 0) {
    SaveActualImage(actual, golden_name);
    FAIL() << "Rendering differs from golden image by " << diff_pixels
           << " pixels. Actual saved to: actual_" << golden_name.toStdString();
  }
}

TEST_F(GraphRenderingTest, MultipleLines) {
  // Use different slopes to make lines visually distinct.
  TestDataSource data_source2(0.5, 20.0);  // slope=0.5, y_offset=20
  Graph graph;
  graph.setFixedSize(400, 300);

  auto* pane = graph.AddPane();
  auto* line1 = pane->plot().AddLine(data_source_);  // slope=1.0 (default)
  line1->SetColor(Qt::blue);
  auto* line2 = pane->plot().AddLine(data_source2);
  line2->SetColor(Qt::red);

  auto data_range = data_source_.GetHorizontalRange();
  graph.horizontal_axis().SetTimeFit(false);
  graph.horizontal_axis().SetRange(data_range);

  QImage actual = RenderWidget(graph);

  const QString golden_name = "multiple_lines.png";
  QImage expected = LoadGoldenImage(golden_name);

  if (expected.isNull()) {
    SaveGoldenImage(actual, golden_name);
    GTEST_SKIP() << "Golden image created. Re-run test to verify.";
  }

  int diff_pixels = CompareImages(actual, expected);

  if (diff_pixels != 0) {
    SaveActualImage(actual, golden_name);
    FAIL() << "Rendering differs from golden image by " << diff_pixels
           << " pixels. Actual saved to: actual_" << golden_name.toStdString();
  }

  // Clean up before data_source2 is destroyed.
  pane->plot().DeleteAllLines();
}

TEST_F(GraphRenderingTest, MultiplePanes) {
  // Use time-based data sources with increasing and decreasing trends.
  // Fixed base time for reproducible rendering.
  const auto base_time = QDateTime::fromMSecsSinceEpoch(1700000000000LL);  // 2023-11-14
  const int count = 100;

  // Pane 1: increasing trend (slope = 1.0)
  TimeTestDataSource data_source1{base_time, std::chrono::hours(1), count,
                                  1.0, 0.0};

  // Pane 2: decreasing trend (slope = -0.8, offset to keep values positive)
  TimeTestDataSource data_source2{base_time, std::chrono::hours(1), count,
                                  -0.8, 100.0};

  Graph graph;
  graph.setFixedSize(400, 300);

  auto* pane1 = graph.AddPane();
  auto* line1 = pane1->plot().AddLine(data_source1);
  line1->SetColor(Qt::blue);

  auto* pane2 = graph.AddPane();
  auto* line2 = pane2->plot().AddLine(data_source2);
  line2->SetColor(Qt::green);

  auto data_range = data_source1.GetHorizontalRange();
  graph.horizontal_axis().SetTimeFit(false);
  graph.horizontal_axis().SetRange(data_range);

  QImage actual = RenderWidget(graph);

  const QString golden_name = "multiple_panes.png";
  QImage expected = LoadGoldenImage(golden_name);

  if (expected.isNull()) {
    SaveGoldenImage(actual, golden_name);
    GTEST_SKIP() << "Golden image created. Re-run test to verify.";
  }

  int diff_pixels = CompareImages(actual, expected);

  if (diff_pixels != 0) {
    SaveActualImage(actual, golden_name);
    FAIL() << "Rendering differs from golden image by " << diff_pixels
           << " pixels. Actual saved to: actual_" << golden_name.toStdString();
  }

  // Clean up before data sources are destroyed.
  graph.DeleteAllPanes();
}

}  // namespace
}  // namespace views
