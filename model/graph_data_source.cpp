#include "graph_qt/model/graph_data_source.h"

#include "graph_qt/model/graph_types.h"

namespace views {

GraphDataSource::GraphDataSource() = default;

GraphDataSource::~GraphDataSource() {
  assert(!observer_);
}

GraphRange GraphDataSource::CalculateAutoRange(double x1, double x2) {
  PointEnumerator* point_enum = EnumPoints(x1, x2, true, false);
  if (!point_enum)
    return GraphRange();

  GraphPoint point;
  if (!point_enum->EnumNext(point))
    return GraphRange();

  double low = point.y;
  double high = point.y;

  while (point_enum->EnumNext(point)) {
    if (point.y < low)
      low = point.y;
    else if (point.y > high)
      high = point.y;
  }

  return GraphRange(low, high);
}

QString GraphDataSource::GetYAxisLabel(double value) const {
  return QString::number(value);
}

}  // namespace views
