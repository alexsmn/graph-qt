#include "graph_qt/graph.h"

#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_widget.h"

#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cfloat>

namespace views {

std::string FormatTime(base::Time time, const char* format_string) {
  if (strcmp(format_string, "ms") == 0) {
    base::Time::Exploded e = {0};
    time.LocalExplode(&e);
    return base::StringPrintf("%d:%02d.%03d", e.minute, e.second,
                              e.millisecond);
  } else {
    char buf[128];
    time_t t = time.ToTimeT();
    tm* ttm = localtime(&t);
    if (!ttm)
      return std::string();
    size_t size = strftime(buf, sizeof(buf), format_string, ttm);
    return std::string(buf, buf + size);
  }
}

// Graph

Graph::Graph()
    : controller_(nullptr),
      state_(STATE_IDLE),
      resizing_pane_(NULL),
      m_time_fit(true),
      right_range_limit_(std::numeric_limits<double>::max()),
      vertical_cursor_label_width_(70),
      selected_cursor_(NULL),
      selected_pane_(NULL),
      grid_pen_(QColor(237, 237, 237)),
      selected_cursor_pen_(QColor(100, 100, 100)),
      selected_cursor_color_(100, 100, 100),
      horizontal_axis_(new GraphAxis) {
  horizontal_axis_->Init(this, NULL, false);
  horizontal_axis_->setParent(this);

  setFrameStyle(QFrame::StyledPanel);
  setStyleSheet("background-color: white;");
}

Graph::~Graph() {
  DeleteAllPanes();
}

void Graph::paintEvent(QPaintEvent* e) {
  QFrame::paintEvent(e);

  QPainter painter(this);

  auto panes_bounds = GetContentsBounds();

  // Draw dividers between panes.
  if (panes_.size() >= 2) {
    Panes::iterator last = --panes_.end();
    for (Panes::iterator i = panes_.begin(); i != last; ++i) {
      GraphPane& pane = **i;
      int y = pane.geometry().bottom();
      painter.drawLine(panes_bounds.x() - 1, y,
                       panes_bounds.right() - kVerticalAxisWidth + 1, y);
    }
  }
}

QRect Graph::GetContentsBounds() const {
  QRect rect(0, 0, width(), height());
  rect.adjust(1, 1, 0, 0);
  return rect;
}

QRect Graph::GetPanesBounds() const {
  QRect bounds = GetContentsBounds();
  bounds.setHeight(std::max(0, bounds.height() - kHorizontalAxisHeight));
  return bounds;
}

void Graph::resizeEvent(QResizeEvent* e) {
  // Normalize percent.
  int total_percent = 0;
  for (Panes::iterator i = panes_.begin(); i != panes_.end(); i++) {
    GraphPane& pane = **i;
    total_percent += pane.size_percent_;
  }
  for (Panes::iterator i = panes_.begin(); i != panes_.end(); i++) {
    GraphPane& pane = **i;
    pane.size_percent_ = pane.size_percent_ * 100 / total_percent;
  }

  auto panes_bounds = GetPanesBounds();

  // Calc new panes bounds.
  int y = panes_bounds.y();
  for (Panes::iterator i = panes_.begin(); i != panes_.end(); i++) {
    GraphPane& pane = **i;

    int size;
    if (&pane == panes_.back())
      size = std::max(0, panes_bounds.bottom() - y);
    else
      size = pane.size_percent_ * panes_bounds.height() / 100;

    QRect bounds(panes_bounds.x(), y, panes_bounds.width(), size);
    pane.setGeometry(bounds);

    y += size + 1;
  }

  // Calculate location of time axis.
  auto contents_bounds = GetContentsBounds();
  QRect horz_axis_bounds(
      contents_bounds.x() - 1, contents_bounds.bottom() - kHorizontalAxisHeight,
      std::max(0, contents_bounds.width() - kVerticalAxisWidth + 2),
      kHorizontalAxisHeight);
  horizontal_axis_->setGeometry(horz_axis_bounds);

  OnHorizontalRangeUpdated();

  update();
}

void Graph::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::RightButton) {
    // delete selected cursor
    if (selected_cursor_)
      DeleteCursor(*selected_cursor_);
    return;
  }

  if (e->button() != Qt::LeftButton)
    return;

  down_point_ = last_point_ = e->pos();
  state_ = STATE_MOUSE_DOWN;

  resizing_pane_ = GetPaneSizerAt(down_point_);
  if (resizing_pane_) {
    state_ = STATE_PANE_RESIZING;
    setCursor(Qt::SizeVerCursor);
  }
}

void Graph::mouseMoveEvent(QMouseEvent* e) {
  switch (state_) {
    case STATE_PANE_RESIZING: {
      assert(resizing_pane_);
      auto panes_bounds = GetPanesBounds();
      int new_size = e->y() - resizing_pane_->y();
      int new_size_percent = new_size * 100 / panes_bounds.height();
      int size_percent_delta = new_size_percent - resizing_pane_->size_percent_;
      resizing_pane_->size_percent_ += size_percent_delta;
      GraphPane* next_pane = GetNextPane(resizing_pane_);
      assert(next_pane);
      next_pane->size_percent_ -= size_percent_delta;
      // Layout();
      if (controller())
        controller()->OnGraphModified();
      break;
    }
  }

  last_point_ = e->pos();
}

void Graph::mouseReleaseEvent(QMouseEvent* e) {
  if (e->button() != Qt::LeftButton)
    return;

  //  if (state_ != STATE_IDLE && state_ != STATE_MOUSE_DOWN)
  //    ReleaseCapture();
  state_ = STATE_IDLE;
}

/*gfx::NativeCursor Graph::GetCursor(const gfx::Point& point) const {
  if (state_ == STATE_PANE_RESIZING || GetPaneSizerAt(point))
    return LoadCursor(NULL, IDC_SIZENS);
  else
    return NULL;
}*/

GraphPane* Graph::GetPaneSizerAt(QPoint point) const {
  for (Panes::const_iterator i = panes_.begin(); i != panes_.end(); i++) {
    GraphPane& pane = **i;
    if (&pane != panes_.back() && abs(point.y() - pane.geometry().bottom()) < 3)
      return &pane;
  }
  return NULL;
}

GraphPane* Graph::GetNextPane(GraphPane* pane) const {
  assert(pane);
  Panes::const_iterator i = std::find(panes_.begin(), panes_.end(), pane);
  if (i == panes_.end())
    return NULL;
  i++;
  if (i == panes_.end())
    return NULL;
  return *i;
}

GraphPane* Graph::GetPrevPane(GraphPane* pane) const {
  assert(pane);
  Panes::const_iterator i = std::find(panes_.begin(), panes_.end(), pane);
  if (i == panes_.begin())
    return NULL;
  i--;
  return *i;
}

void Graph::DeletePane(GraphPane& pane) {
  // Remove from zooming history.
  for (ZoomingHistory::iterator i = zooming_history_.begin();
       i != zooming_history_.end(); ++i) {
    ZoomingHistoryItem& item = *i;
    if (item.pane_ == &pane)
      item.pane_ = NULL;
  }

  if (&pane == selected_pane_)
    SelectPane(NULL);

  // delete pane from chart
  Panes::iterator pi = std::find(panes_.begin(), panes_.end(), &pane);
  assert(pi != panes_.end());
  panes_.erase(pi);

  // delete itself
  delete &pane;
}

void Graph::SelectPane(GraphPane* pane) {
  if (pane == selected_pane_)
    return;

  //	if (selected_pane_)
  //		InvalidatePane(selected_pane_);
  selected_pane_ = pane;
  //	if (selected_pane_)
  //		InvalidatePane(selected_pane_);

  if (controller_)
    controller_->OnGraphSelectPane();
}

void Graph::InvalidateCursor(const GraphCursor& cursor) {
  if (cursor.axis_->is_vertical()) {
    cursor.axis_->plot()->InvalidateCursor(cursor);

  } else {
    for (Panes::iterator i = panes_.begin(); i != panes_.end(); ++i)
      (*i)->plot().InvalidateCursor(cursor);
  }

  cursor.axis_->InvalidateCursor(cursor);
}

void Graph::MoveCursor(const GraphCursor& cursor, double position) {
  //::UpdateWindow(GetWindowHandle());

  InvalidateCursor(cursor);
  const_cast<GraphCursor&>(cursor).position_ = position;
  InvalidateCursor(cursor);

  if (selected_cursor_ == &cursor)
    UpdateCurBox();
}

void Graph::SelectCursor(const GraphCursor* cursor) {
  if (selected_cursor_)
    InvalidateCursor(*selected_cursor_);

  selected_cursor_ = const_cast<GraphCursor*>(cursor);

  if (selected_cursor_)
    InvalidateCursor(*selected_cursor_);

  UpdateCurBox();
}

void Graph::DeleteCursor(const GraphCursor& cursor) {
  InvalidateCursor(cursor);

  if (selected_cursor_ == &cursor)
    selected_cursor_ = NULL;

  cursor.axis_->DeleteCursor(cursor);

  UpdateCurBox();
}

void Graph::AdjustTimeRange() {
  auto range = horizontal_axis_->range();
  AdjustTimeRange(range);
  horizontal_axis_->SetRange(range);
}

void Graph::AdjustTimeRange(GraphRange& range) {
  for (const auto* pane : panes_) {
    for (const auto* line : pane->plot().lines())
      line->AdjustTimeRange(range);
  }
}

void Graph::OnHorizontalRangeUpdated() {
  UpdateAutoRanges();
}

void Graph::UpdateAutoRanges() {
  for (Panes::iterator i = panes_.begin(); i != panes_.end(); ++i) {
    GraphPane& pane = **i;
    const GraphPlot::Lines& lines = pane.plot().lines();
    for (GraphPlot::Lines::const_iterator i = lines.begin(); i != lines.end();
         ++i)
      (*i)->UpdateAutoRange();
  }
}

QString Graph::GetCursorLabel(const GraphCursor& cursor) const {
  if (!cursor.axis_->is_vertical())
    return GetXAxisLabel(cursor.position_);
  else {
    GraphLine* line = cursor.axis_->plot()->primary_line();
    if (line && line->data_source())
      return line->data_source()->GetYAxisLabel(cursor.position_);
    else
      return QString();
  }
}

QString Graph::GetXAxisLabel(double val) const {
  static const double kSecondStep = 1.0;
  static const double kMinuteStep = 60 * kSecondStep;
  static const double kHourStep = 60 * kMinuteStep;
  static const double kDayStep = 24 * kHourStep;

  // time format
  const char* format_string;
  if (horizontal_axis_->tick_step() >= kDayStep)
    format_string = "%#d %b";
  else if (horizontal_axis_->tick_step() >= kHourStep)
    format_string = "%#d-%#H:%M";
  else if (horizontal_axis_->tick_step() >= kMinuteStep)
    format_string = "%#H:%M";
  else if (horizontal_axis_->tick_step() >= kSecondStep)
    format_string = "%#H:%M:%S";
  else
    format_string = "ms";  // special msec format

  return QString::fromStdString(
      FormatTime(base::Time::FromDoubleT(val), format_string));
}

void Graph::AddPane(GraphPane& pane) {
  pane.Init(*this);
  pane.setParent(this);
  panes_.push_back(&pane);
}

void Graph::DeleteAllPanes() {
  for (Panes::iterator i = panes_.begin(); i != panes_.end(); i++)
    delete *i;
  panes_.clear();
}

void Graph::Zoom(GraphPane& pane,
                 const GraphRange& horizontal_range,
                 const GraphRange& vertical_range) {
  // ZoomingHistoryItem history_item =
  //     { horizontal_range_, &pane, pane.range() };
  // zooming_history_.push_back(history_item);

  pane.vertical_axis().SetRange(vertical_range);
  horizontal_axis().SetRange(horizontal_range);
}

void Graph::Fit() {
  auto range = horizontal_axis().range();

  if (m_time_fit && right_range_limit_ != std::numeric_limits<double>::max()) {
    range = views::GraphRange{right_range_limit_ - range.delta(),
                              right_range_limit_, range.kind()};
  }

  AdjustTimeRange(range);
  horizontal_axis().SetRange(range);
}

/*View* Graph::GetEventHandlerForPoint(const QPoint& point) {
  if (state_ == STATE_PANE_RESIZING || GetPaneSizerAt(point))
    return this;

  return __super::GetEventHandlerForPoint(point);
}

void Graph::OnFocusChanged(View* focused_before, View* focused_now) {
  if (!focused_now)
    return;

  for (Panes::iterator i = panes_.begin(); i != panes_.end(); ++i) {
    GraphPane& pane = **i;
    if (pane.Contains(focused_now)) {
      SelectPane(&pane);
      break;
    }
  }
}

void Graph::ViewHierarchyChanged(const ViewHierarchyChangedDetails& details) {
  if (details.child == this) {
    FocusManager* focus_manager = GetFocusManager();
    if (focus_manager) {
      if (details.is_add)
        focus_manager->AddFocusChangeListener(*this);
      else
        focus_manager->RemoveFocusChangeListener(*this);
    }
  }
}

void Graph::RequestFocus() {
  if (selected_pane_)
    selected_pane_->RequestFocus();
  else
    View::RequestFocus();
}*/

}  // namespace views
