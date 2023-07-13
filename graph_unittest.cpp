#include "graph_qt/graph.h"

#include "graph_qt/graph_axis.h"
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
      GraphRange(view_range.high() - view_range.delta(), view_range.high()));
}

}  // namespace views