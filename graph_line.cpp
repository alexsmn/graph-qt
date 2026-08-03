#include "graph_qt/graph_line.h"

#include "graph_qt/graph.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/model/graph_data_source.h"

#include <QFontMetrics>
#include <QPainter>
#include <algorithm>
#include <cfloat>

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

GraphLine::GraphLine() = default;

GraphLine::~GraphLine() {
  if (data_source_) {
    data_source_->SetObserver(nullptr);
  }
}

void GraphLine::SetDataSource(GraphDataSource* data_source) {
  if (data_source_ == data_source) {
    return;
  }

  if (data_source_) {
    data_source_->SetObserver(nullptr);
  }

  data_source_ = data_source;

  if (data_source_) {
    data_source_->SetObserver(this);
  }

  SetCurrentValue(data_source_ ? data_source_->GetCurrentValue()
                               : kGraphUnknownValue);

  UpdateHorizontalRange();
  UpdateVerticalRange();

  // Values changed, need to invalidate.
  if (plot_) {
    plot_->update();
  }
}

void GraphLine::SetLineWeight(int line_weight) {
  line_weight = std::clamp(line_weight, 1, 10);
  if (line_weight_ == line_weight) {
    return;
  }

  line_weight_ = line_weight;
  if (plot_) {
    plot_->update();
  }
}

void GraphLine::SetLimitStyle(LimitBand band, const LimitStyle& style) {
  limit_styles_[static_cast<int>(band)] = style;
  if (plot_) {
    plot_->update();
  }
}

void GraphLine::ClearLimitStyles() {
  for (LimitStyle& style : limit_styles_) {
    style = LimitStyle{};
  }
  if (plot_) {
    plot_->update();
  }
}

void GraphLine::DrawLimit(QPainter& painter,
                          const QRect& rect,
                          double limit,
                          const LimitStyle& style) const {
  if (limit == kGraphUnknownValue) {
    return;
  }
  const int y = ValueToY(limit);
  const QColor color = style.color.isValid() ? style.color : color_;
  painter.save();
  painter.setPen(QPen(QBrush(color), 1, Qt::DashLine));
  painter.drawLine(rect.x(), y, rect.right(), y);
  if (!style.label.isEmpty()) {
    // Solid caption sitting just above the line at the right edge.
    painter.setPen(color);
    const QFontMetrics metrics(painter.font());
    const int text_width = metrics.horizontalAdvance(style.label);
    painter.drawText(rect.right() - text_width - 4, y - 3, style.label);
  }
  painter.restore();
}

void GraphLine::Draw(QPainter& painter, const QRect& rect) {
  if (!data_source_) {
    return;
  }

  // get range from screen
  double x1 = XToValue(rect.x());
  double x2 = XToValue(rect.right());

  QBrush brush(color_);

  GraphPoint value;
  auto point_enum = data_source_->EnumPoints(x1, x2, true, true);
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

  // Limit bands: DrawLimit skips unset bands and falls back to the series
  // colour when a band has no explicit style, so this reproduces the historical
  // series-coloured dashed limits until a caller sets per-band styles.
  DrawLimit(painter, rect, data_source_->limit_lolo_,
            limit_styles_[static_cast<int>(LimitBand::kLoLo)]);
  DrawLimit(painter, rect, data_source_->limit_lo_,
            limit_styles_[static_cast<int>(LimitBand::kLo)]);
  DrawLimit(painter, rect, data_source_->limit_hi_,
            limit_styles_[static_cast<int>(LimitBand::kHi)]);
  DrawLimit(painter, rect, data_source_->limit_hihi_,
            limit_styles_[static_cast<int>(LimitBand::kHiHi)]);
}

void GraphLine::SetCurrentValue(double value) {
  if (current_value_ == value) {
    return;
  }

  if (plot_ && current_value_ != kGraphUnknownValue) {
    plot_->vertical_axis().InvalidateCurrentValue(current_value_);
  }

  current_value_ = value;

  if (plot_ && current_value_ != kGraphUnknownValue) {
    plot_->vertical_axis().InvalidateCurrentValue(current_value_);
  }
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

GraphRange GraphLine::CalculateVerticalAutoRange() {
  assert(auto_range());

  if (!plot_ || !data_source_) {
    return GraphRange();
  }

  // Get range from screen.
  double x1 = XToValue(0);
  double x2 = XToValue(plot().width());

  return data_source_->CalculateAutoRange(x1, x2);
}

bool GraphLine::GetNearestPoint(const QPoint& screen_point,
                                GraphPoint& data_point,
                                int max_distance) {
  if (!data_source_) {
    return false;
  }

  // get range from screen
  double x1 = XToValue(0);
  double x2 = XToValue(plot().width());

  auto point_enum = data_source_->EnumPoints(x1, x2, true, false);
  if (!point_enum) {
    return false;
  }

  GraphPoint point;
  if (!point_enum->EnumNext(point)) {
    return false;
  }

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

void GraphLine::SetVerticalRange(const GraphRange& range) {
  set_auto_range(false);
  SetVerticalRangeHelper(range);
}

void GraphLine::AdjustHorizontalRange(GraphRange& range) const {
  {
    auto points =
        data_source_->EnumPoints(range.low(), range.high(), false, false);
    if (!points || points->GetCount() <= kMaxPoints) {
      return;
    }
  }

  // Binary search for low bound to keep kMaxPoints points.

  double min = range.low_;
  double max = range.high_;

  for (;;) {
    double value = (min + max) / 2;
    auto points = data_source_->EnumPoints(value, range.high_, false, false);
    auto count = points ? points->GetCount() : 0;

    // Allow 5% error.
    if (count <= kMaxPoints && kMaxPoints - count <= kMaxPoints / 20) {
      range.low_ = value;
      return;
    }

    if (count < kMaxPoints) {
      max = value;
    } else {
      min = value;
    }
  }
}

void GraphLine::SetVerticalRangeHelper(const GraphRange& range) {
  if (vertical_range_ == range) {
    return;
  }

  vertical_range_ = range;

  if (plot_) {
    plot_->vertical_axis().UpdateRange();
  }
}

void GraphLine::OnDataSourceItemChanged() {
  UpdateHorizontalRange();
  UpdateVerticalRange();

  // Values changed, need to invalidate.
  if (plot_) {
    plot_->update();
  }
}

void GraphLine::OnDataSourceHistoryChanged() {
  UpdateHorizontalRange();
  UpdateVerticalRange();

  // Values changed, need to invalidate.
  if (plot_) {
    plot_->update();
  }
}

void GraphLine::OnDataSourceCurrentValueChanged() {
  SetCurrentValue(data_source_->GetCurrentValue());
}

void GraphLine::UpdateHorizontalRange() {
  if (!plot_ || !data_source_) {
    return;
  }

  auto range = data_source_->GetHorizontalRange();
  plot_->graph().horizontal_axis().SetScrollRange(
      plot_->graph().horizontal_axis().scroll_range().combine(range));
}

void GraphLine::UpdateVerticalRange() {
  SetVerticalRangeHelper(auto_range()   ? CalculateVerticalAutoRange()
                         : data_source_ ? data_source_->GetVerticalRange()
                                        : GraphRange{});
}

void GraphLine::SetColor(const QColor& color) {
  if (color_ == color) {
    return;
  }

  color_ = color;

  if (plot_) {
    plot_->update();
  }
}

GraphRange GraphLine::GetHorizontalRange() const {
  return data_source_ ? data_source_->GetHorizontalRange() : GraphRange{};
}

}  // namespace views
