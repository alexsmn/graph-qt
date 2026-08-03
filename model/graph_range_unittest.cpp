#include "graph_qt/model/graph_range.h"

#include <gtest/gtest.h>

namespace views {

class GraphRangeTest : public ::testing::Test {};

TEST_F(GraphRangeTest, DefaultConstruction) {
  GraphRange range;
  EXPECT_EQ(range.kind(), GraphRange::LINEAR);
}

TEST_F(GraphRangeTest, Construction) {
  GraphRange range{10.0, 20.0};

  EXPECT_EQ(range.low(), 10.0);
  EXPECT_EQ(range.high(), 20.0);
  EXPECT_EQ(range.delta(), 10.0);
  EXPECT_EQ(range.kind(), GraphRange::LINEAR);
}

TEST_F(GraphRangeTest, ConstructionWithKind) {
  GraphRange range{0.0, 100.0, GraphRange::TIME};

  EXPECT_EQ(range.kind(), GraphRange::TIME);
}

TEST_F(GraphRangeTest, Empty) {
  GraphRange empty{5.0, 5.0};
  GraphRange non_empty{5.0, 10.0};

  EXPECT_TRUE(empty.empty());
  EXPECT_FALSE(non_empty.empty());
}

TEST_F(GraphRangeTest, Contains) {
  GraphRange range{10.0, 20.0};

  EXPECT_TRUE(range.Contains(10.0));
  EXPECT_TRUE(range.Contains(15.0));
  EXPECT_TRUE(range.Contains(20.0));
  EXPECT_FALSE(range.Contains(9.9));
  EXPECT_FALSE(range.Contains(20.1));
}

TEST_F(GraphRangeTest, Offset) {
  GraphRange range{10.0, 20.0};
  range.Offset(5.0);

  EXPECT_EQ(range.low(), 15.0);
  EXPECT_EQ(range.high(), 25.0);
}

TEST_F(GraphRangeTest, OffsetChangesLogicalToLinear) {
  GraphRange range = GraphRange::Logical();
  EXPECT_EQ(range.kind(), GraphRange::LOGICAL);

  range.Offset(1.0);
  EXPECT_EQ(range.kind(), GraphRange::LINEAR);
}

TEST_F(GraphRangeTest, HighSubrange) {
  GraphRange range{0.0, 100.0};
  auto sub = range.high_subrange(30.0);

  EXPECT_EQ(sub.low(), 70.0);
  EXPECT_EQ(sub.high(), 100.0);
}

TEST_F(GraphRangeTest, HighSubrangePreservesKind) {
  GraphRange range{0.0, 100.0, GraphRange::TIME};
  auto sub = range.high_subrange(30.0);

  EXPECT_EQ(sub.kind(), GraphRange::TIME);
}

TEST_F(GraphRangeTest, HighSubrangeEmptyReturnsOriginal) {
  GraphRange empty{50.0, 50.0};
  auto sub = empty.high_subrange(10.0);

  EXPECT_EQ(sub, empty);
}

TEST_F(GraphRangeTest, Combine) {
  GraphRange a{10.0, 30.0};
  GraphRange b{20.0, 50.0};

  auto combined = a.combine(b);

  EXPECT_EQ(combined.low(), 10.0);
  EXPECT_EQ(combined.high(), 50.0);
}

TEST_F(GraphRangeTest, CombineWithEmpty) {
  GraphRange range{10.0, 30.0};
  GraphRange empty{20.0, 20.0};

  EXPECT_EQ(range.combine(empty), range);
  EXPECT_EQ(empty.combine(range), range);
}

TEST_F(GraphRangeTest, Logical) {
  auto range = GraphRange::Logical();

  EXPECT_EQ(range.low(), 0.0);
  EXPECT_EQ(range.high(), 1.0);
  EXPECT_EQ(range.kind(), GraphRange::LOGICAL);
}

TEST_F(GraphRangeTest, SetTime) {
  GraphRange range{0.0, 100.0};
  EXPECT_EQ(range.kind(), GraphRange::LINEAR);

  range.set_time();
  EXPECT_EQ(range.kind(), GraphRange::TIME);
}

TEST_F(GraphRangeTest, Equality) {
  GraphRange a{10.0, 20.0};
  GraphRange b{10.0, 20.0};
  GraphRange c{10.0, 30.0};
  GraphRange d{10.0, 20.0, GraphRange::TIME};

  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
  EXPECT_NE(a, d);
}

}  // namespace views
