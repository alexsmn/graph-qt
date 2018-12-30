#include "graph_qt/graph_line.h"

#include <qpainter.h>
#include <cfloat>

#include "base/win/scoped_gdi_object.h"
#include "graph_qt/graph.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/model/graph_data_source.h"

namespace views {

namespace {
// Correlates to the screen resolution.
const size_t kMaxPoints = 10000;
}  // namespace

template <typename T>
inline T sqr(T x) {
  return x * x;
}

inline int CalcPointDistance(const QPoint& a, const QPoint& b) {
  return static_cast<int>(
      sqrt(static_cast<double>(sqr(a.x() - b.x()) + sqr(a.y() - b.y()))));
}

GraphLine::GraphLine()
    : plot_(NULL),
      data_source_(NULL),
      flags(STEPPED | AUTO_RANGE | SHOW_DOTS),
      line_weight_(1),
      current_value_(kGraphUnknownValue) {}

GraphLine::~GraphLine() {
  if (data_source_)
    data_source_->SetObserver(this);
}

void GraphLine::SetDataSource(GraphDataSource* data_source) {
  if (data_source_ == data_source)
    return;

  if (data_source_)
    data_source_->SetObserver(NULL);

  data_source_ = data_source;

  if (data_source_)
    data_source_->SetObserver(this);

  SetCurrentValue(data_source_ ? data_source_->current_value()
                               : kGraphUnknownValue);

  UpdateRange();

  // Values changed, need to invalidate.
  if (plot_)
    plot_->update();
}

void GraphLine::DrawLimit(QPainter& painter,
                          const QRect& rect,
                          double limit,
                          const QPen& pen) {
  int y = ValueToY(limit);
  // TODO: Set pen preliminary.
  painter.save();
  painter.setPen(pen);
  painter.drawLine(rect.x(), y, rect.right(), y);
  painter.restore();
}

void GraphLine::Draw(QPainter& painter, const QRect& rect) {
  if (!data_source_)
    return;

  // get range from screen
  double x1 = XToValue(rect.x());
  double x2 = XToValue(rect.right());

  QBrush brush(color_);

  GraphPoint value;
  PointEnumerator* point_enum = data_source_->EnumPoints(x1, x2, true, true);
  if (point_enum && point_enum->EnumNext(value)) {
    // select pen
    QPen solid_pen(brush, line_weight_);
    QPen dash_pen(brush, 1, Qt::DotLine);

    // Draw points.

    QPoint last_point(ValueToX(value.x), ValueToY(value.y));
    painter.setPen(value.good ? solid_pen : dash_pen);

    while (point_enum->EnumNext(value)) {
      // current point
      QPoint point(ValueToX(value.x), ValueToY(value.y));
      /*if (smooth()) {
        PolyBezierTo(canvas->native_canvas(), &pt, 1);
      } else*/
      {
        if (stepped()) {
          QPoint corner_point(point.x(), last_point.y());
          painter.drawLine(last_point, corner_point);
          painter.drawLine(corner_point, point);
        } else {
          painter.drawLine(last_point, point);
        }
      }

      // Draw dot on previous point (current draw on current as it will overlap
      // line).
      if (dots_shown()) {
        QRect dot_rect(last_point.x() - line_weight_,
                       last_point.y() - line_weight_, line_weight_ * 2 + 1,
                       line_weight_ * 2 + 1);
        painter.fillRect(dot_rect, color_);
      }

      painter.setPen(value.good ? solid_pen : dash_pen);
      last_point = point;
    }

    // Draw last dot.
    if (dots_shown()) {
      QRect dot_rect(last_point.x() - line_weight_,
                     last_point.y() - line_weight_, line_weight_ * 2 + 1,
                     line_weight_ * 2 + 1);
      painter.fillRect(dot_rect, color_);
    }
  }

  QPen limits_pen(brush, 1, Qt::DashLine);
  if (data_source_->limit_hi_ != kGraphUnknownValue)
    DrawLimit(painter, rect, data_source_->limit_hi_, limits_pen);
  if (data_source_->limit_lo_ != kGraphUnknownValue)
    DrawLimit(painter, rect, data_source_->limit_lo_, limits_pen);
  if (data_source_->limit_hihi_ != kGraphUnknownValue)
    DrawLimit(painter, rect, data_source_->limit_hihi_, limits_pen);
  if (data_source_->limit_lolo_ != kGraphUnknownValue)
    DrawLimit(painter, rect, data_source_->limit_lolo_, limits_pen);
}

void GraphLine::SetCurrentValue(double value) {
  if (current_value_ == value)
    return;

  if (current_value_ != kGraphUnknownValue)
    plot().vertical_axis().InvalidateCurrentValue(current_value_);

  current_value_ = value;

  if (current_value_ != kGraphUnknownValue)
    plot().vertical_axis().InvalidateCurrentValue(current_value_);
}

double GraphLine::XToValue(int x) const {
  return plot().horizontal_axis().ConvertScreenToValue(x);
}

double GraphLine::YToValue(int y) const {
  return plot().vertical_axis().ConvertScreenToValue(y);
}

int GraphLine::ValueToX(double value) const {
  return plot().horizontal_axis().ConvertValueToScreen(value);
}

int GraphLine::ValueToY(double value) const {
  return plot().vertical_axis().ConvertValueToScreen(value);
}

GraphRange GraphLine::CalculateAutoRange() {
  assert(auto_range());

  if (!plot_ || !data_source_)
    return GraphRange();

  // Get range from screen.
  double x1 = XToValue(0);
  double x2 = XToValue(plot().width());

  return data_source_->CalculateAutoRange(x1, x2);
}

bool GraphLine::GetNearestPoint(const QPoint& screen_point,
                                GraphPoint& data_point,
                                int max_distance) {
  if (!data_source_)
    return false;

  // get range from screen
  double x1 = XToValue(0);
  double x2 = XToValue(plot().width());

  PointEnumerator* point_enum = data_source_->EnumPoints(x1, x2, true, false);
  if (!point_enum)
    return false;

  GraphPoint point;
  if (!point_enum->EnumNext(point))
    return false;

  QPoint p(ValueToX(point.x), ValueToY(point.y));
  int min_distance = CalcPointDistance(p, screen_point);
  data_point = point;

  while (point_enum->EnumNext(point)) {
    QPoint p(ValueToX(point.x), ValueToY(point.y));
    int distance = CalcPointDistance(p, screen_point);
    if (distance < min_distance) {
      data_point = point;
      min_distance = distance;
    }
  }

  return min_distance <= max_distance;
}

void GraphLine::SetRange(const GraphRange& range) {
  set_auto_range(false);
  SetRangeHelper(range);
}

void GraphLine::UpdateAutoRange() {
  if (!auto_range())
    return;

  GraphRange range = CalculateAutoRange();
  SetRangeHelper(range);
}

void GraphLine::AdjustTimeRange(GraphRange& range) const {
  auto* points =
      data_source_->EnumPoints(range.low(), range.high(), false, false);
  if (!points || points->GetCount() <= kMaxPoints)
    return;

  // Binary search for low bound to keep kMaxPoints points.

  double min = range.low_;
  double max = range.high_;

  for (;;) {
    double value = (min + max) / 2;
    auto* points = data_source_->EnumPoints(value, range.high_, false, false);
    auto count = points ? points->GetCount() : 0;

    // Allow 5% error.
    if (count <= kMaxPoints && kMaxPoints - count <= kMaxPoints / 20) {
      range.low_ = value;
      return;
    }

    if (count < kMaxPoints)
      max = value;
    else
      min = value;
  }
}

void GraphLine::SetRangeHelper(const GraphRange& range) {
  if (range_ == range)
    return;

  range_ = range;

  if (plot_)
    plot_->vertical_axis().UpdateRange();
}

void GraphLine::OnDataSourceItemChanged() {
  UpdateRange();

  if (plot_) {
    plot_->graph().AdjustTimeRange();

    plot_->update();
  }
}

void GraphLine::OnDataSourceHistoryChanged() {
  UpdateRange();

  if (plot_) {
    plot_->graph().AdjustTimeRange();

    // Values changed, need to invalidate.
    plot_->update();
  }
}

void GraphLine::OnDataSourceCurrentValueChanged() {
  SetCurrentValue(data_source_->current_value());
}

void GraphLine::UpdateRange() {
  if (auto_range())
    UpdateAutoRange();
  else
    SetRange(data_source_ ? data_source_->range() : GraphRange());
}

void GraphLine::SetColor(QColor color) {
  if (color_ == color)
    return;

  color_ = color;

  if (plot_)
    plot_->update();
}

}  // namespace views
