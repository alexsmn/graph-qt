#include "graph_qt/graph.h"

#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_time_helper.h"
#include "graph_qt/test/golden_image.h"
#include "test/test_data_source.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDeadlineTimer>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QStyle>
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

  std::unique_ptr<PointEnumerator> EnumPoints(
      double from,
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

// Counts paint events delivered to the widget it is installed on.
class PaintCounter : public QObject {
 public:
  int count = 0;

 protected:
  bool eventFilter(QObject* object, QEvent* event) override {
    if (event->type() == QEvent::Paint)
      ++count;
    return QObject::eventFilter(object, event);
  }
};

// Pumps the event loop until `predicate` holds, or the deadline passes.
// Condition-driven rather than a fixed number of passes: a repaint is
// delivered when Qt gets to it, so counting passes makes the test a
// measurement of the machine.
template <typename Predicate>
bool DrainUntil(Predicate&& predicate,
                std::chrono::milliseconds timeout = std::chrono::seconds{5}) {
  QDeadlineTimer deadline{timeout};
  while (!predicate()) {
    if (deadline.hasExpired())
      return predicate();
    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
  }
  return true;
}

// Returns the path to the testdata directory.
QString GetTestDataPath() {
  QDir dir{QFileInfo{QString::fromUtf8(__FILE__)}.absoluteDir()};
  return dir.filePath("testdata");
}

// Renders a widget to a QImage.
QImage RenderWidget(QWidget& widget) {
  widget.setFixedSize(400, 300);
  widget.show();
  // Process events to ensure layout is complete.
  QCoreApplication::processEvents();
  auto image = widget.grab().toImage();
  if (image.size() != widget.size()) {
    image = image.scaled(widget.size(), Qt::IgnoreAspectRatio,
                         Qt::SmoothTransformation);
  }
  return image;
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
    QApplication::setStyle("Fusion");
    QApplication::setPalette(QApplication::style()->standardPalette());
    testdata_path_ = GetTestDataPath();
  }

  // Compares `actual` against the golden image `name` in testdata, creating
  // the golden when none exists yet and skipping the test so the next run
  // verifies against it.
  //
  // A golden that exists but does not decode is a failure, never a missing
  // golden: it means the baseline is damaged, and regenerating one there would
  // replace a reviewed image with whatever the current code renders.
  void ExpectMatchesGolden(const QImage& actual, const QString& name) {
    QDir().mkpath(testdata_path_);
    const QString path = testdata_path_ + "/" + name;

    QImage expected;
    switch (test::LoadGoldenImage(path, expected)) {
      case test::GoldenLoadResult::kLoaded:
        break;
      case test::GoldenLoadResult::kAbsent:
        ASSERT_TRUE(test::SaveGoldenImage(actual, path))
            << "Failed to save golden image: " << path.toStdString();
        GTEST_SKIP() << "Golden image created. Re-run test to verify.";
      case test::GoldenLoadResult::kUnreadable:
        FAIL() << "Golden image exists but cannot be decoded: "
               << path.toStdString()
               << ". Restore it from git rather than regenerating it: this "
                  "build may simply be missing the image codec.";
    }

    const int diff_pixels = CompareImages(actual, expected);
    if (diff_pixels == 0) {
      return;
    }

#if defined(Q_OS_MACOS)
    GTEST_SKIP() << "Golden rendering is platform-specific on macOS.";
#else
    // Debug output only, and deliberately not a golden path: a failed write
    // here costs nothing but a missing artifact.
    const QString actual_path = testdata_path_ + "/actual_" + name;
    actual.save(actual_path);
    FAIL() << "Rendering differs from golden image by " << diff_pixels
           << " pixels. Actual saved to: " << actual_path.toStdString();
#endif
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

  ExpectMatchesGolden(actual, "basic_graph.png");
}

TEST_F(GraphRenderingTest, DarkBackgroundUsesLightGraphChrome) {
  Graph graph;

  QPalette palette = graph.palette();
  palette.setColor(graph.backgroundRole(), QColor{20, 20, 20});
  graph.setPalette(palette);

  EXPECT_GT(graph.text_color().red(), 200);
  EXPECT_GT(graph.cursor_color().green(), 200);
  EXPECT_GT(graph.grid_pen().color().blue(), 60);
  EXPECT_LT(graph.grid_pen().color().blue(), graph.text_color().blue());
  EXPECT_EQ(Graph::ContrastTextColor(QColor{0, 0, 160}), QColor(245, 245, 245));
}

TEST_F(GraphRenderingTest, LightBackgroundUsesDarkGraphChrome) {
  Graph graph;

  QPalette palette = graph.palette();
  palette.setColor(graph.backgroundRole(), QColor{240, 240, 240});
  graph.setPalette(palette);

  EXPECT_LT(graph.text_color().red(), 80);
  EXPECT_LT(graph.cursor_color().green(), 80);
  EXPECT_LT(graph.grid_pen().color().blue(), 240);
  EXPECT_EQ(Graph::ContrastTextColor(QColor{240, 240, 240}),
            QColor(25, 25, 25));
}

// The canvas used to be hardwired to Qt::white in the constructor, which left a
// white plot in a dark window on any dark-appearance desktop. It is a data
// surface, so it takes QPalette::Base and follows the palette instead.
TEST_F(GraphRenderingTest, CanvasFollowsBaseRole) {
  Graph graph;

  EXPECT_EQ(graph.backgroundRole(), QPalette::Base);

  QPalette dark_palette = graph.palette();
  dark_palette.setColor(QPalette::Base, QColor{18, 18, 18});
  graph.setPalette(dark_palette);

  EXPECT_EQ(graph.background_color(), QColor(18, 18, 18));
  EXPECT_GT(graph.text_color().red(), 200);
}

// The panes, plots and axes paint from the Graph's colours rather than from
// their own palettes, so a palette change has to reach them or an OS
// light/dark switch leaves the chart chrome drawn for the previous appearance.
// Qt gives this for free today — children inherit the palette, so propagation
// repaints them — but only for as long as no child sets a palette of its own.
// This test is the guard on that.
TEST_F(GraphRenderingTest, PaletteChangeRepaintsChildren) {
  Graph graph;
  graph.setFixedSize(400, 300);
  auto* pane = graph.AddPane();
  pane->plot().AddLine(data_source_);

  PaintCounter counter;
  pane->plot().installEventFilter(&counter);

  graph.show();
  // Wait for the FIRST paint rather than assuming show() plus one pass
  // delivered it: Qt does not repaint a widget that is not yet exposed, so a
  // palette change applied before that point schedules nothing and the
  // assertion below reads a counter that was never going to move.
  ASSERT_TRUE(DrainUntil([&] { return counter.count > 0; }))
      << "the plot never painted after show()";
  counter.count = 0;

  QPalette dark_palette = graph.palette();
  dark_palette.setColor(QPalette::Base, QColor{18, 18, 18});
  graph.setPalette(dark_palette);

  // A palette change schedules a deferred repaint; how many event-loop passes
  // it takes to arrive is not something the test controls. One
  // processEvents() call happened to be enough most of the time and not all of
  // it -- measured at 4 failures in 10 serial runs on an idle machine, every
  // one of them `counter.count > 0` reading 0 -- which read as -j contention
  // and was not.
  EXPECT_TRUE(DrainUntil([&] { return counter.count > 0; }))
      << "the palette change did not repaint the plot";
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

  ExpectMatchesGolden(actual, "multiple_lines.png");

  // Clean up before data_source2 is destroyed.
  pane->plot().DeleteAllLines();
}

TEST_F(GraphRenderingTest, MultiplePanes) {
  // Use time-based data sources with increasing and decreasing trends.
  // Fixed base time for reproducible rendering.
  const auto base_time =
      QDateTime::fromMSecsSinceEpoch(1700000000000LL);  // 2023-11-14
  const int count = 100;

  // Pane 1: increasing trend (slope = 1.0)
  TimeTestDataSource data_source1{base_time, std::chrono::hours(1), count, 1.0,
                                  0.0};

  // Pane 2: decreasing trend (slope = -0.8, offset to keep values positive)
  TimeTestDataSource data_source2{base_time, std::chrono::hours(1), count, -0.8,
                                  100.0};

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

  ExpectMatchesGolden(actual, "multiple_panes.png");

  // Clean up before data sources are destroyed.
  graph.DeleteAllPanes();
}

}  // namespace
}  // namespace views
