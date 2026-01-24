#include "graph_qt/graph.h"

#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "test/test_data_source.h"

#include <gmock/gmock.h>

using namespace ::testing;

namespace views {

class GraphTest : public Test {
 protected:
  // The data source must outlive the graph.
  TestDataSource data_source_;

  Graph graph_;
};

TEST_F(GraphTest, AddPane) {
  auto* pane = graph_.AddPane();
  EXPECT_THAT(graph_.panes(), ElementsAre(pane));
}

TEST_F(GraphTest, AddLine) {
  auto* pane = graph_.AddPane();
  auto* line = pane->plot().AddLine(data_source_);
  EXPECT_THAT(pane->plot().lines(), ElementsAre(line));
}

TEST_F(GraphTest, TimeFit) {
  auto* pane = graph_.AddPane();
  pane->plot().AddLine(data_source_);

  auto data_range = data_source_.GetHorizontalRange();
  // View only 10% of the whole data range.
  GraphRange view_range{data_range.low(),
                        data_range.low() + data_range.delta() / 10};

  graph_.horizontal_axis().SetRange(view_range);
  graph_.horizontal_axis().SetTimeFit(true);

  // Expect that the viewed range was shifted to the high bound.
  EXPECT_EQ(
      graph_.horizontal_axis().range(),
      GraphRange(data_range.high() - view_range.delta(), data_range.high()));
}

TEST_F(GraphTest, AddMultiplePanes) {
  auto* pane1 = graph_.AddPane();
  auto* pane2 = graph_.AddPane();
  auto* pane3 = graph_.AddPane();

  EXPECT_THAT(graph_.panes(), ElementsAre(pane1, pane2, pane3));
}

TEST_F(GraphTest, DeletePane) {
  auto* pane1 = graph_.AddPane();
  auto* pane2 = graph_.AddPane();
  auto* pane3 = graph_.AddPane();

  graph_.DeletePane(*pane2);

  EXPECT_THAT(graph_.panes(), ElementsAre(pane1, pane3));
}

TEST_F(GraphTest, DeleteAllPanes) {
  graph_.AddPane();
  graph_.AddPane();
  graph_.AddPane();

  graph_.DeleteAllPanes();

  EXPECT_TRUE(graph_.panes().empty());
}

TEST_F(GraphTest, GetNextPane) {
  auto* pane1 = graph_.AddPane();
  auto* pane2 = graph_.AddPane();
  auto* pane3 = graph_.AddPane();

  EXPECT_EQ(graph_.GetNextPane(pane1), pane2);
  EXPECT_EQ(graph_.GetNextPane(pane2), pane3);
  EXPECT_EQ(graph_.GetNextPane(pane3), nullptr);
}

TEST_F(GraphTest, GetPrevPane) {
  auto* pane1 = graph_.AddPane();
  auto* pane2 = graph_.AddPane();
  auto* pane3 = graph_.AddPane();

  EXPECT_EQ(graph_.GetPrevPane(pane1), nullptr);
  EXPECT_EQ(graph_.GetPrevPane(pane2), pane1);
  EXPECT_EQ(graph_.GetPrevPane(pane3), pane2);
}

TEST_F(GraphTest, SelectPane) {
  auto* pane1 = graph_.AddPane();
  auto* pane2 = graph_.AddPane();

  EXPECT_EQ(graph_.selected_pane(), nullptr);

  graph_.SelectPane(pane1);
  EXPECT_EQ(graph_.selected_pane(), pane1);

  graph_.SelectPane(pane2);
  EXPECT_EQ(graph_.selected_pane(), pane2);

  graph_.SelectPane(nullptr);
  EXPECT_EQ(graph_.selected_pane(), nullptr);
}

TEST_F(GraphTest, AddMultipleLines) {
  TestDataSource data_source2;
  auto* pane = graph_.AddPane();

  auto* line1 = pane->plot().AddLine(data_source_);
  auto* line2 = pane->plot().AddLine(data_source2);

  EXPECT_THAT(pane->plot().lines(), ElementsAre(line1, line2));
  EXPECT_EQ(pane->plot().primary_line(), line1);

  // Clean up lines before local data_source2 is destroyed.
  pane->plot().DeleteAllLines();
}

TEST_F(GraphTest, DeleteLine) {
  TestDataSource data_source2;
  auto* pane = graph_.AddPane();

  auto* line1 = pane->plot().AddLine(data_source_);
  auto* line2 = pane->plot().AddLine(data_source2);

  pane->plot().DeleteLine(*line1);

  EXPECT_THAT(pane->plot().lines(), ElementsAre(line2));
  EXPECT_EQ(pane->plot().primary_line(), line2);

  // Clean up remaining line before local data_source2 is destroyed.
  pane->plot().DeleteAllLines();
}

TEST_F(GraphTest, DeleteAllLines) {
  TestDataSource data_source2;
  auto* pane = graph_.AddPane();

  pane->plot().AddLine(data_source_);
  pane->plot().AddLine(data_source2);

  pane->plot().DeleteAllLines();

  EXPECT_TRUE(pane->plot().lines().empty());
  EXPECT_EQ(pane->plot().primary_line(), nullptr);
}

TEST_F(GraphTest, LineFlags) {
  auto* pane = graph_.AddPane();
  auto* line = pane->plot().AddLine(data_source_);

  // Default flags: stepped, auto_range, dots_shown are true; smooth is false.
  EXPECT_TRUE(line->stepped());
  EXPECT_TRUE(line->auto_range());
  EXPECT_TRUE(line->dots_shown());
  EXPECT_FALSE(line->smooth());

  line->set_stepped(false);
  EXPECT_FALSE(line->stepped());

  line->set_auto_range(false);
  EXPECT_FALSE(line->auto_range());

  line->set_dots_shown(false);
  EXPECT_FALSE(line->dots_shown());

  line->set_smooth(true);
  EXPECT_TRUE(line->smooth());
}

TEST_F(GraphTest, LineColor) {
  auto* pane = graph_.AddPane();
  auto* line = pane->plot().AddLine(data_source_);

  EXPECT_EQ(line->color(), Qt::black);

  line->SetColor(Qt::red);
  EXPECT_EQ(line->color(), Qt::red);
}

TEST_F(GraphTest, LineDataSource) {
  auto* pane = graph_.AddPane();
  auto* line = pane->plot().AddLine(data_source_);

  EXPECT_EQ(line->data_source(), &data_source_);
}

TEST_F(GraphTest, AddCursor) {
  graph_.AddPane();

  const auto& cursor = graph_.horizontal_axis().AddCursor(500.0);

  EXPECT_EQ(cursor.position_, 500.0);
  EXPECT_EQ(graph_.horizontal_axis().cursors().size(), 1u);
}

TEST_F(GraphTest, SelectCursor) {
  graph_.AddPane();

  const auto& cursor = graph_.horizontal_axis().AddCursor(500.0);

  EXPECT_EQ(graph_.selected_cursor(), nullptr);

  graph_.SelectCursor(&cursor);
  EXPECT_EQ(graph_.selected_cursor(), &cursor);

  graph_.SelectCursor(nullptr);
  EXPECT_EQ(graph_.selected_cursor(), nullptr);
}

TEST_F(GraphTest, MoveCursor) {
  graph_.AddPane();

  const auto& cursor = graph_.horizontal_axis().AddCursor(500.0);
  graph_.MoveCursor(cursor, 750.0);

  EXPECT_EQ(cursor.position_, 750.0);
}

TEST_F(GraphTest, DeleteCursor) {
  graph_.AddPane();

  // Add both cursors first, then get reference to avoid vector reallocation
  // invalidating the reference.
  graph_.horizontal_axis().AddCursor(500.0);
  graph_.horizontal_axis().AddCursor(600.0);

  EXPECT_EQ(graph_.horizontal_axis().cursors().size(), 2u);

  graph_.DeleteCursor(graph_.horizontal_axis().cursors()[0]);

  EXPECT_EQ(graph_.horizontal_axis().cursors().size(), 1u);
  EXPECT_EQ(graph_.horizontal_axis().cursors()[0].position_, 600.0);
}

TEST_F(GraphTest, HorizontalAxisRange) {
  graph_.AddPane();

  GraphRange range{100.0, 200.0};
  graph_.horizontal_axis().SetRange(range);

  EXPECT_EQ(graph_.horizontal_axis().range(), range);
}

TEST_F(GraphTest, TimeFitDisabled) {
  auto* pane = graph_.AddPane();
  pane->plot().AddLine(data_source_);

  auto data_range = data_source_.GetHorizontalRange();
  GraphRange view_range{data_range.low(),
                        data_range.low() + data_range.delta() / 10};

  graph_.horizontal_axis().SetTimeFit(false);
  graph_.horizontal_axis().SetRange(view_range);

  // With time fit disabled, the range should remain as set.
  EXPECT_EQ(graph_.horizontal_axis().range(), view_range);
}

}  // namespace views