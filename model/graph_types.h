#pragma once

#include <limits>

namespace views {

using GraphValue = double;

const GraphValue kGraphUnknownValue = std::numeric_limits<GraphValue>::min();

struct GraphPoint {
  GraphPoint() = default;
  GraphPoint(GraphValue x, GraphValue y) : x(x), y(y) {}

  bool operator==(const GraphPoint& other) const = default;

  GraphValue x = 0.0;
  GraphValue y = 0.0;
  bool good = false;
};

}  // namespace views
