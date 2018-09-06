#include "graph_qt/model/graph_data_source.h"

#include "graph_qt/model/graph_types.h"

namespace views {

GraphDataSource::GraphDataSource()
    : limit_lo_(kGraphUnknownValue),
      limit_hi_(kGraphUnknownValue),
      limit_lolo_(kGraphUnknownValue),
      limit_hihi_(kGraphUnknownValue),
      current_value_(kGraphUnknownValue),
      observer_(NULL) {
}

GraphDataSource::~GraphDataSource() {
  assert(!observer_);
}

void GraphDataSource::SetCurrentValue(double value) {
  if (current_value_ == value)
    return;
    
  current_value_ = value;

  if (observer_)
    observer_->OnDataSourceCurrentValueChanged();    
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

} // namespace views
