#include "graph_qt/graph_axis.h"

#include "base/auto_reset.h"
#include "base/time/time.h"
#include "graph_qt/graph.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_time_helper.h"
#include "graph_qt/model/graph_data_source.h"

#include <QDateTime>
#include <QMouseEvent>
#include <QPainter>

namespace views {

namespace {

static const int kLabelHeight = 20;

double EstimateValueTickStep(int area_sy, double delta, int min) {
  if (area_sy <= 0)
    return 1.0;
  double step = delta / 10.0;
  double f = pow(10.0, floor(log10(step)));
  step = floor(step / f) * f;
  while (step * 2.0 * area_sy / delta < min)
    step *= 2.0;
  while (step / 2.0 * area_sy / delta > min)
    step /= 2.0;
  return step;
}

double EstimateTimeTickStep(double scale, int min) {
  assert(scale >= 0.0);
  static const double times[] = {
      GraphTimeHelper::msec,       GraphTimeHelper::msec * 5,
      GraphTimeHelper::msec * 10,  GraphTimeHelper::msec * 50,
      GraphTimeHelper::msec * 100, GraphTimeHelper::msec * 500,
      GraphTimeHelper::sec,        5 * GraphTimeHelper::sec,
      5 * GraphTimeHelper::sec,    15 * GraphTimeHelper::sec,
      30 * GraphTimeHelper::sec,   GraphTimeHelper::min,
      5 * GraphTimeHelper::min,    15 * GraphTimeHelper::min,
      30 * GraphTimeHelper::min,   GraphTimeHelper::hour,
      2 * GraphTimeHelper::hour,   3 * GraphTimeHelper::hour,
      4 * GraphTimeHelper::hour,   6 * GraphTimeHelper::hour,
      12 * GraphTimeHelper::hour,  GraphTimeHelper::day};
  if (scale <= 0)
    return times[_countof(times) - 1];
  for (int i = 0; i < _countof(times); i++) {
    int sx = (int)(times[i] * scale);
    if (sx >= min)
      return times[i];
  }
  return times[_countof(times) - 1];
}

void Inset(QRect& rect, int left, int top, int right, int bottom) {
  int new_left = rect.left() + left;
  int new_top = rect.top() + top;
  int new_right = rect.right() - right;
  int new_bottom = rect.bottom() - bottom;
  rect.setRect(new_left, new_top, std::max(0, new_right - new_left),
               std::max(0, new_bottom - new_top));
}

}  // namespace

QString GetTimeAxisLabel(double val, double tick_step) {
  static const double kSecondStep = 1.0;
  static const double kMinuteStep = 60 * kSecondStep;
  static const double kHourStep = 60 * kMinuteStep;
  static const double kDayStep = 24 * kHourStep;

  auto date_time =
      QDateTime::fromMSecsSinceEpoch(base::Time::FromDoubleT(val).ToJavaTime());

  if (tick_step >= kDayStep) {
    return date_time.toString("d MMM");
  } else if (tick_step >= kHourStep) {
    return date_time.toString("d-hh:mm");
  } else if (tick_step >= kMinuteStep) {
    return date_time.toString("h:mm");
  } else if (tick_step >= kSecondStep) {
    return date_time.toString("h:mm:ss");
  } else {
    return date_time.toString("m:ss.zzz");
  }
}

// GraphAxis

GraphAxis::GraphAxis(QWidget* parent) : QWidget{parent} {
  setMouseTracking(true);
  CalcDrawRect();
}

GraphAxis::~GraphAxis() = default;

void GraphAxis::Init(Graph* graph, GraphPlot* plot, bool is_vertical) {
  // The `plot` is null for the horizontal axis.
  assert(graph);

  graph_ = graph;
  plot_ = plot;
  is_vertical_ = is_vertical;
}

int GraphAxis::GetSize() const {
  return is_vertical_ ? draw_rc.height() : draw_rc.width();
}

void GraphAxis::GetTickValues(double& first_value, double& last_value) const {
  int first_pos = is_vertical_ ? height() : 0;
  int last_pos = is_vertical_ ? 0 : width();

  first_value = ConvertScreenToValue(first_pos);
  first_value = first_value - fmod(first_value, tick_step_);

  last_value = ConvertScreenToValue(last_pos);
}

void GraphAxis::paintEvent(QPaintEvent* e) {
  assert(graph_);

  if (range_.empty())
    return;

  QPainter painter(this);

  // Draw axis line.
  /*if (is_vertical_)
    painter.drawLine(0, 0, 0, height());
  else
    painter.drawLine(0, 0, width(), 0);*/

  double first_value, last_value;
  GetTickValues(first_value, last_value);

  // TODO: Draw values inside clip rect only.
  for (double v = first_value; v <= last_value; v += tick_step_) {
    int p = ConvertValueToScreen(v);

    PaintTick(painter, p);

    auto label = GetLabelForValue(v);
    PaintLabel(painter, p, label);
  }

  if (is_vertical_) {
    for (const auto* line : plot_->lines()) {
      PaintCurrentValue(painter, *line);
    }
  }

  for (const auto& cursor : cursors_) {
    PaintCursorLabel(painter, cursor);
  }
}

void GraphAxis::PaintTick(QPainter& painter, int pos) {
  if (is_vertical_)
    painter.drawLine(0, pos, 3, pos);
  else
    painter.drawLine(pos, 0, pos, 4);
}

void GraphAxis::PaintLabel(QPainter& painter, int pos, const QString& label) {
  if (is_vertical_) {
    QRect bounds(5, pos, 0, 0);
    painter.drawText(
        bounds, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip, label);

  } else {
    QRect bounds(pos, 3, 0, 0);
    painter.drawText(bounds, Qt::AlignHCenter | Qt::AlignTop | Qt::TextDontClip,
                     label);
  }
}

void GraphAxis::PaintCurrentValue(QPainter& painter, const GraphLine& line) {
  if (!line.data_source())
    return;

  double value = line.current_value();
  if (value == kGraphUnknownValue)
    return;

  auto rect = GetCurrentValueRect(value);

  painter.fillRect(rect, line.color());

  rect.adjust(5, 0, 0, 0);

  auto label = line.data_source()->GetYAxisLabel(value);
  painter.setPen(Qt::white);
  painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, label);
}

const GraphCursor& GraphAxis::AddCursor(double position) {
  cursors_.push_back(GraphCursor(position, this));
  GraphCursor& cursor = cursors_.back();
  graph_->InvalidateCursor(cursor);
  return cursor;
}

void GraphAxis::DeleteCursor(const GraphCursor& cursor) {
  for (Cursors::iterator i = cursors_.begin(); i != cursors_.end(); ++i) {
    if (&*i == &cursor) {
      cursors_.erase(i);
      return;
    }
  }
  assert(false);
}

QRect GraphAxis::GetCursorLabelRect(const GraphCursor& cursor) const {
  assert(cursor.axis_ == this);
  assert(graph_);

  if (is_vertical_) {
    int y = ConvertValueToScreen(cursor.position_);
    return QRect(0, y - kLabelHeight / 2, width(), kLabelHeight);

  } else {
    int x = ConvertValueToScreen(cursor.position_);
    return QRect(x - graph_->vertical_cursor_label_width_ / 2 - 1, 0,
                 graph_->vertical_cursor_label_width_, height());
  }
}

double GraphAxis::ConvertScreenToValue(int pos) const {
  if (is_vertical_) {
    if (draw_rc.height() == 0)
      return range_.low();
    return range_.low() +
           (draw_rc.bottom() - pos) * range_.delta() / draw_rc.height();
  } else {
    if (width() == 0)
      return range_.low();
    return range_.low() +
           (pos - draw_rc.x()) * range_.delta() / draw_rc.width();
  }
}

int GraphAxis::ConvertValueToScreen(double value) const {
  if (is_vertical_) {
    if (range_.empty())
      return draw_rc.bottom();
    return draw_rc.bottom() - (int)floor((value - range_.low()) *
                                         draw_rc.height() / range_.delta());
  } else {
    if (range_.empty())
      return draw_rc.x();
    return draw_rc.x() + (int)floor((value - range_.low()) * draw_rc.width() /
                                    range_.delta());
  }
}

QString GraphAxis::GetLabelForValue(double value) const {
  assert(graph_);

  if (is_vertical_) {
    GraphLine* line = plot_->primary_line();
    return line && line->data_source()
               ? line->data_source()->GetYAxisLabel(value)
               : QString();

  } else {
    return graph_->GetXAxisLabel(value);
  }
}

void GraphAxis::CalcDrawRect() {
  draw_rc = QRect(0, 0, width(), height());

  // Calculate step of axis and grid.
  double range = range_.delta();
  if (is_vertical_) {
    Inset(draw_rc, 0, Graph::kDrawingRectOffsetY, 0,
          Graph::kDrawingRectOffsetY);

    if (range_.kind() == GraphRange::LOGICAL)
      tick_step_ = range;
    else
      tick_step_ = EstimateValueTickStep(draw_rc.height(), range, 30);

  } else {
    Inset(draw_rc, Graph::kDrawingRectOffsetX + 1, 0,
          Graph::kDrawingRectOffsetX + 1, 0);

    // TODO: Check |range| for zero.
    double factor = draw_rc.width() / range;
    tick_step_ = EstimateTimeTickStep(factor, 50);
  }
}

void GraphAxis::resizeEvent(QResizeEvent* e) {
  assert(graph_);

  CalcDrawRect();
  update();
}

void GraphAxis::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::RightButton) {
    ignore_context_menu_ = false;
    if (graph_->selected_cursor()) {
      graph_->DeleteCursor(*graph_->selected_cursor());
      ignore_context_menu_ = true;
    }
    return;
  }

  if (graph_->selected_cursor_) {
    graph_->SelectCursor(nullptr);
    return;
  }

  const GraphCursor* cursor = GetCursorLabelAt(event->pos());
  if (cursor) {
    graph_->SelectCursor(cursor);
    return;
  }

  moved_ = false;
  last_point_ = event->pos();
}

void GraphAxis::mouseMoveEvent(QMouseEvent* event) {
  assert(graph_);

  if (event->buttons() & Qt::LeftButton) {
    if (!is_vertical_) {
      moved_ = true;

      int dx = event->x() - last_point_.x();
      double dt = dx * range_.delta() / width();
      GraphRange range(range_.low() - dt, range_.high(), range_.kind());
      graph_->AdjustTimeRange(range);
      SetRange(range);

      if (graph_->controller())
        graph_->controller()->OnGraphModified();
    }

    last_point_ = event->pos();

  } else {
    if (graph_->selected_cursor_ && graph_->selected_cursor_->axis_ == this) {
      double position =
          ConvertScreenToValue(is_vertical_ ? event->y() : event->x());
      graph_->MoveCursor(*graph_->selected_cursor_, position);
      moved_ = true;
    }
  }
}

void GraphAxis::mouseReleaseEvent(QMouseEvent* event) {
  assert(graph_);

  if (event->button() != Qt::LeftButton)
    return;

  if (!moved_) {
    double position =
        ConvertScreenToValue(is_vertical_ ? event->y() : event->x());
    const GraphCursor& cursor = AddCursor(position);
    graph_->SelectCursor(&cursor);
  }
}

void GraphAxis::contextMenuEvent(QContextMenuEvent* event) {
  assert(graph_);

  if (!ignore_context_menu_)
    QWidget::contextMenuEvent(event);
}

const GraphCursor* GraphAxis::GetCursorLabelAt(QPoint point) const {
  for (auto i = cursors_.begin(); i != cursors_.end(); ++i) {
    const GraphCursor& cursor = *i;
    auto rect = GetCursorLabelRect(cursor);
    if (rect.contains(point))
      return &cursor;
  }

  return nullptr;
}

void GraphAxis::PaintCursorLabel(QPainter& painter, const GraphCursor& cursor) {
  auto color = &cursor == graph_->selected_cursor_
                   ? graph_->selected_cursor_color_
                   : Qt::black;

  auto rect = GetCursorLabelRect(cursor);
  painter.fillRect(rect, color);

  auto label = graph_->GetCursorLabel(cursor);
  painter.setPen(Qt::white);
  painter.drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, label);
}

QRect GraphAxis::GetCurrentValueRect(double value) const {
  assert(is_vertical_);
  assert(graph_);
  assert(value != kGraphUnknownValue);

  int y = ConvertValueToScreen(value);
  return QRect(0, y - kLabelHeight / 2, width(), kLabelHeight);
}

void GraphAxis::InvalidateCurrentValue(double value) {
  update(GetCurrentValueRect(value));
}

void GraphAxis::UpdateRange() {
  const GraphPlot::Lines& lines = plot_->lines();
  if (lines.empty())
    return;

  double low = kGraphUnknownValue;
  double high = kGraphUnknownValue;
  bool logical = false;

  auto i = lines.begin();
  const GraphLine& line = **i;
  low = line.vertical_range().low();
  high = line.vertical_range().high();
  logical = line.vertical_range().kind() == GraphRange::LOGICAL;

  ++i;
  for (; i != lines.end(); ++i) {
    const GraphLine& line = **i;
    const GraphRange& line_range = line.vertical_range();

    logical &= line_range.kind() == GraphRange::LOGICAL;

    if (line_range.low() != kGraphUnknownValue) {
      if (low == kGraphUnknownValue || line_range.low() < low)
        low = line_range.low();
    }

    if (line_range.high() != kGraphUnknownValue) {
      if (high == kGraphUnknownValue || line_range.high() > high)
        high = line_range.high();
    }
  }

  SetRange(logical ? GraphRange::Logical() : GraphRange(low, high));
}

void GraphAxis::SetRange(const GraphRange& range) {
  if (range_ == range)
    return;

  auto adjusted_range = range;
  graph_->AdjustTimeRange(adjusted_range);

  if (range_ == adjusted_range)
    return;

  range_ = adjusted_range;

  // Update tick step.
  CalcDrawRect();

  if (is_vertical_) {
    // TODO: Don't use pane.
    plot_->update();
  }

  if (!time_fit_updating_) {
    time_fit_ = false;
  }

  emit rangeChanged(adjusted_range);
}

void GraphAxis::InvalidateCursor(const GraphCursor& cursor) {
  update(GetCursorLabelRect(cursor));
}

void GraphAxis::Fit() {
  if (!graph_) {
    return;
  }

  auto range = this->range();
  if (range.empty()) {
    return;
  }

  if (time_fit_ && !scroll_range_.empty()) {
    range = scroll_range_.high_subrange(range.delta());
  }

  base::AutoReset updating{&time_fit_updating_, true};
  SetRange(range);
}

void GraphAxis::SetTimeFit(bool time_fit) {
  if (time_fit_ == time_fit) {
    return;
  }

  time_fit_ = time_fit;

  if (time_fit_) {
    Fit();
  }
}

void GraphAxis::SetScrollRange(const GraphRange& range) {
  if (range == scroll_range_) {
    return;
  }

  scroll_range_ = range;

  if (time_fit_) {
    Fit();
  }

  emit scrollRangeChanged(range);
}

}  // namespace views
