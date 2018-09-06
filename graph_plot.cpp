#include "graph_qt/graph_plot.h"

#include <qevent.h>
#include <qpainter.h>
#include <limits>

#include "base/strings/sys_string_conversions.h"
#include "base/strings/stringprintf.h"
#include "graph_qt/graph.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_widget.h"

QRect MakeRectFromPoints(const QPoint& a, const QPoint& b) {
  return QRect(std::min(a.x(), b.x()), std::min(a.y(), b.y()),
               std::abs(a.x() - b.x()), std::abs(a.y() - b.y()));
}

namespace views {

static const int kCursorWidth = 2;

GraphPlot::GraphPlot()
    : graph_(NULL),
      pane_(NULL),
      horizontal_axis_(NULL),
      vertical_axis_(NULL),
      state_(STATE_IDLE),
      zooming_(false),
      focus_line_(NULL) {
  setMouseTracking(true);
}

GraphPlot::~GraphPlot() {
  DeleteAllLines();
}

void GraphPlot::Init(Graph* graph, GraphPane* pane, GraphAxis* horizontal_axis,
                     GraphAxis* vertical_axis) {
  graph_ = graph;
  pane_ = pane;
  horizontal_axis_ = horizontal_axis;
  vertical_axis_ = vertical_axis;
}

void GraphPlot::AddLine(GraphLine& line) {
  assert(!line.plot_);
  
  line.plot_ = this;
  lines_.push_back(&line);

  update();
  vertical_axis().update();
}

void GraphPlot::DeleteLine(GraphLine& line) {
  if (&line == focus_line_)
    SetFocusPoint(GraphPoint(), NULL);

  // Delete from list.
  Lines::iterator i = std::find(lines_.begin(), lines_.end(), &line);
  assert(i != lines_.end());
  lines_.erase(i);

  // Delete instance.
  delete &line;

  update();
  vertical_axis().update();
}

void GraphPlot::DeleteAllLines() {
  while (!lines_.empty())
    DeleteLine(*lines_.back());

  update();
  vertical_axis().update();
}

void GraphPlot::paintEvent(QPaintEvent* e) {
  QPainter painter(this);

  // draw value axis and grid
  PaintHorizontalGrid(painter);
  PaintVerticalGrid(painter);

  // draw lines
  {
    QRect local_bounds(0, 0, width(), height());
    for (Lines::const_iterator i = lines_.begin(); i != lines_.end(); ++i)
      (*i)->Draw(painter, local_bounds);
  }

  // Draw horizontal cursors.
  for (Cursors::const_iterator i = vertical_axis_->cursors().begin();
                               i != vertical_axis_->cursors().end(); ++i) {
    PaintCursor(painter, *i);
  }

  // Draw vertical cursors.
  for (Cursors::const_iterator i = horizontal_axis_->cursors().begin();
                               i != horizontal_axis_->cursors().end(); ++i) {
    PaintCursor(painter, *i);
  }

  // Draw focus point.
  if (focus_line_) {
    QPoint p(focus_line_->ValueToX(focus_point_.x),
             focus_line_->ValueToY(focus_point_.y));
    painter.fillRect(QRect(p.x() - 3, p.y() - 3, 6, 6), focus_line_->color);
  }
}

void GraphPlot::PaintHorizontalGrid(QPainter& painter) {
  assert(graph_);

  if (vertical_axis_->range().empty())
    return;

  painter.save();
  painter.setPen(graph_->grid_pen_);

  double first_value, last_value;
  vertical_axis_->GetTickValues(first_value, last_value);

  double grid_step = vertical_axis_->tick_step();
    
  for (double v = first_value; v <= last_value; v += grid_step) {
    int p = vertical_axis_->ConvertValueToScreen(v);
    painter.drawLine(1, p, width() - 1, p);
  }

  painter.restore();
}

void GraphPlot::mousePressEvent(QMouseEvent* e) {
  setFocus(Qt::MouseFocusReason);

  if (!(e->buttons() & Qt::LeftButton))
    return;

  down_point_ = last_point_ = e->pos();
  state_ = STATE_MOUSE_DOWN;
  e->accept();
}

void GraphPlot::mouseMoveEvent(QMouseEvent* e) {
  auto delta = e->pos() - last_point_;

  if (state_ == STATE_MOUSE_DOWN && down_point_ != e->pos()) {
    if (zooming_)
      state_ = STATE_ZOOMING;
    else
      state_ = STATE_PANNING;
  }

  switch (state_) {
    case STATE_PANNING:
      if (delta.x()) {
        graph_->m_time_fit = false;

        const GraphRange& range = horizontal_axis_->range();
        double dt = -delta.x() * range.delta() / width();
        if (graph_->right_range_limit_ != std::numeric_limits<double>::max() &&
            range.high() + dt > graph_->right_range_limit_) {
          dt = graph_->right_range_limit_ - range.high();
        }

        GraphRange new_range = range;
        new_range.Offset(dt);
        horizontal_axis_->SetRange(new_range);
      }
      break;
      
    case STATE_ZOOMING: {
      QPainter painter(this);
      PaintZoomRect(painter);
      last_point_ = e->pos();
      PaintZoomRect(painter);
      break;
    }

    default: {
      bool clear_focus = true;
      GraphLine* line = primary_line();
      if (line) {
        GraphPoint v;
        if (line->GetNearestPoint(e->pos(), v, 15)) {
          SetFocusPoint(v, line);
          clear_focus = false;
        }
      }
      if (clear_focus)
        SetFocusPoint(GraphPoint(), NULL);
      return;
    }
  }

  last_point_ = e->pos();
  e->accept();
}

void GraphPlot::mouseReleaseEvent(QMouseEvent* e) {
  if (state_ == STATE_ZOOMING) {
    QPainter painter(this);
    PaintZoomRect(painter);
    
    //GraphRange horizontal_range, vertical_range;
    //horizontal_range.low = XToValue(down_point_.x);
    //horizontal_range.high = XToValue(last_point_.x);
    //if (horizontal_range.low > horizontal_range.high)
    //  std::swap(horizontal_range.low, horizontal_range.high);

    //vertical_range.low = YToValue(down_point_.y);
    //vertical_range.high = YToValue(last_point_.y);
    //if (vertical_range.low > vertical_range.high)
    //  std::swap(vertical_range.low, vertical_range.high);

    //graph_->Zoom(*this, horizontal_range, vertical_range);
  }

  state_ = STATE_IDLE;
  e->accept();
}

void GraphPlot::leaveEvent(QEvent* e) {
  SetFocusPoint(GraphPoint(), NULL);
}

void GraphPlot::PaintZoomRect(QPainter& painter) {
  QRect rect = MakeRectFromPoints(down_point_, last_point_);

  painter.save();
  painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
  painter.drawRect(rect);
  painter.restore();
}

void GraphPlot::AddWidget(GraphWidget& widget) {
  widget.setParent(this);

  widgets_.push_back(&widget);
}

void GraphPlot::RemoveWidget(GraphWidget& widget) {
  widget.setParent(nullptr);

  Widgets::iterator i = std::find(widgets_.begin(), widgets_.end(), &widget);
  assert(i != widgets_.end());
  widgets_.erase(i);
}

void GraphPlot::InvalidateCursor(const GraphCursor& cursor) {
  int pos = cursor.axis_->ConvertValueToScreen(cursor.position_);

  QRect cursor_rect = cursor.axis_->is_vertical() ?
      QRect(0, pos - kCursorWidth / 2, width(), kCursorWidth) :
      QRect(pos - kCursorWidth / 2, 0, kCursorWidth, height());
  update(cursor_rect);
}

void GraphPlot::PaintCursor(QPainter& painter, const GraphCursor& cursor) {
  if (!cursor.axis_->range().Contains(cursor.position_))
    return;

  painter.save();
  painter.setPen(QPen(Qt::black));

  int pos = cursor.axis_->ConvertValueToScreen(cursor.position_);
  if (cursor.axis_->is_vertical())
    painter.drawLine(0, pos, width(), pos);
  else
    painter.drawLine(pos, 0, pos, height());

  painter.restore();
}

void GraphPlot::PaintVerticalGrid(QPainter& painter) {
  if (horizontal_axis_->range().empty())
    return;

  painter.save();
  painter.setPen(graph_->grid_pen_);

  double first_value, last_value;
  horizontal_axis().GetTickValues(first_value, last_value);

  double grid_step = horizontal_axis().tick_step() / 2;
  first_value -= grid_step;
    
  for (double x = first_value; x <= last_value; x += grid_step) {
    int p = horizontal_axis().ConvertValueToScreen(x);
    painter.drawLine(p, 1, p, height() - 1);
  }

  painter.restore();
}

void GraphPlot::SetFocusPoint(const GraphPoint& point, GraphLine* line) {
  if (focus_line_ == line && focus_point_ == point)
    return;

  InvalidateFocusPoint();

  focus_line_ = line;
  focus_point_ = point;
  focus_tooltip_.clear();

  if (focus_line_) {
    auto x_label = graph_->GetXAxisLabel(focus_point_.x);
    auto y_label = focus_line_->data_source() ?
        focus_line_->data_source()->GetYAxisLabel(focus_point_.y) :
        QString();
    focus_tooltip_ = x_label + QStringLiteral("\n") + y_label;
  }

  InvalidateFocusPoint();
}

void GraphPlot::InvalidateFocusPoint() {
  if (!focus_line_)
    return;

  QPoint p(focus_line_->ValueToX(focus_point_.x),
           focus_line_->ValueToY(focus_point_.y));
  update(QRect(p.x() - 3, p.y() - 3, 6, 6));
}

/*bool GraphPlot::GetTooltipText(const QPoint& p, base::string16* tooltip) const {
  if (focus_tooltip_.empty())
    return false;

  *tooltip = focus_tooltip_;
  return true;
}*/

} // namespace views
