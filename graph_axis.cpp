#include "graph_qt/graph_axis.h"

#include <QMouseEvent>
#include <QPainter>

#include "graph_qt/graph.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_time.h"
#include "graph_qt/model/graph_data_source.h"

namespace views {

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
  static const double times[] = {
      GraphTime::msec,      GraphTime::msec * 5,   GraphTime::msec * 10,
      GraphTime::msec * 50, GraphTime::msec * 100, GraphTime::msec * 500,
      GraphTime::sec,       5 * GraphTime::sec,    5 * GraphTime::sec,
      15 * GraphTime::sec,  30 * GraphTime::sec,   GraphTime::min,
      5 * GraphTime::min,   15 * GraphTime::min,   30 * GraphTime::min,
      GraphTime::hour,      2 * GraphTime::hour,   3 * GraphTime::hour,
      4 * GraphTime::hour,  6 * GraphTime::hour,   12 * GraphTime::hour,
      GraphTime::day};
  if (scale <= 0)
    return times[_countof(times) - 1];
  for (int i = 0; i < _countof(times); i++) {
    int sx = (int)(times[i] * scale);
    if (sx >= min)
      return times[i];
  }
  return times[_countof(times) - 1];
}

GraphAxis::GraphAxis()
    : graph_(NULL),
      plot_(NULL),
      is_vertical_(false),
      tick_step_(0.0),
      moved_(false) {
  setMouseTracking(true);
}

GraphAxis::~GraphAxis() {}

void GraphAxis::Init(Graph* graph, GraphPlot* plot, bool is_vertical) {
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
  if (is_vertical_)
    painter.drawLine(0, 0, 0, height());
  else
    painter.drawLine(0, 0, width(), 0);

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
    const GraphPlot::Lines& lines = plot_->lines();
    for (GraphPlot::Lines::const_iterator i = lines.begin(); i != lines.end();
         ++i) {
      PaintCurrentValue(painter, **i);
    }
  }

  for (Cursors::iterator i = cursors_.begin(); i != cursors_.end(); ++i)
    PaintCursorLabel(painter, *i);
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
    QRect bounds(pos, 2, 0, 0);
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
  if (!painter.clipRegion().intersects(rect))
    return;

  painter.fillRect(rect, line.color);

  rect.adjust(5, 0, 0, 0);

  auto label = line.data_source()->GetYAxisLabel(value);
  // TODO: text color
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
    return QRect(1, y - kLabelHeight / 2, std::max(0, width() - 1),
                 kLabelHeight);

  } else {
    int x = ConvertValueToScreen(cursor.position_);
    return QRect(x - graph_->vertical_cursor_label_width_ / 2 - 1, 1,
                 graph_->vertical_cursor_label_width_,
                 std::max(0, height() - 1));
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

void GraphAxis::resizeEvent(QResizeEvent* e) {
  assert(graph_);

  draw_rc = QRect(0, 0, width(), height());

  // Calculate step of axis and grid.
  double range = range_.delta();
  if (is_vertical_) {
    draw_rc.adjust(0, Graph::kDrawingRectOffsetY, 0,
                   -Graph::kDrawingRectOffsetY);

    if (range_.kind() == GraphRange::LOGICAL)
      tick_step_ = range;
    else
      tick_step_ = EstimateValueTickStep(draw_rc.height(), range, 30);

  } else {
    draw_rc.adjust(Graph::kDrawingRectOffsetX + 1, 0,
                   -Graph::kDrawingRectOffsetX - 1, 0);

    // TODO: Check |range| for zero.
    double factor = draw_rc.width() / range;
    tick_step_ = EstimateTimeTickStep(factor, 50);
  }

  update();
}

void GraphAxis::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::RightButton) {
    if (graph_->selected_cursor())
      graph_->DeleteCursor(*graph_->selected_cursor());
    return;
  }

  if (graph_->selected_cursor_) {
    graph_->SelectCursor(NULL);
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

const GraphCursor* GraphAxis::GetCursorLabelAt(QPoint point) const {
  for (Cursors::const_iterator i = cursors_.begin(); i != cursors_.end(); ++i) {
    const GraphCursor& cursor = *i;
    auto rect = GetCursorLabelRect(cursor);
    if (rect.contains(point))
      return &cursor;
  }

  return NULL;
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
  return QRect(1, y - kLabelHeight / 2, std::max(0, width() - 1), kLabelHeight);
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

  GraphPlot::Lines::const_iterator i = lines.begin();
  const GraphLine& line = **i;
  low = line.range().low();
  high = line.range().high();
  logical = line.range().kind() == GraphRange::LOGICAL;

  ++i;
  for (; i != lines.end(); ++i) {
    const GraphLine& line = **i;
    const GraphRange& line_range = line.range();

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

  range_ = range;

  // Update tick step.
  updateGeometry();

  if (is_vertical_) {
    // TODO: Don't use pane.
    plot_->update();

  } else {
    graph_->update();
    graph_->OnHorizontalRangeUpdated();
  }
}

void GraphAxis::InvalidateCursor(const GraphCursor& cursor) {
  update(GetCursorLabelRect(cursor));
}

}  // namespace views
