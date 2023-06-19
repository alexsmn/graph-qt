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
#include <QSplitter>
#include <algorithm>
#include <cfloat>

namespace views {

// Graph

Graph::Graph()
    : controller_(nullptr),
      vertical_cursor_label_width_(70),
      selected_cursor_(NULL),
      selected_pane_(NULL),
      grid_pen_(QColor(237, 237, 237)),
      selected_cursor_color_(100, 100, 100),
      horizontal_axis_(new GraphAxis),
      splitter_{new QSplitter{this}} {
  horizontal_axis_->Init(this, NULL, false);
  horizontal_axis_->setParent(this);

  splitter_->setOrientation(Qt::Vertical);
  splitter_->setHandleWidth(0);

  setFrameStyle(QFrame::StyledPanel);
  setStyleSheet("background-color: white;");
}

Graph::~Graph() {
  DeleteAllPanes();
}

QRect Graph::GetContentsBounds() const {
  QRect rect(0, 0, width(), height());
  rect.adjust(1, 1, 0, 0);
  return rect;
}

QRect Graph::GetPanesBounds() const {
  QRect bounds = GetContentsBounds();
  bounds.setHeight(std::max(0, bounds.height() - kHorizontalAxisHeight - 1));
  return bounds;
}

void Graph::resizeEvent(QResizeEvent* e) {
  splitter_->setGeometry(GetPanesBounds());

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
  return QString::number(val);
}

void Graph::AddPane(GraphPane& pane) {
  pane.Init(*this);
  pane.setParent(this);
  panes_.push_back(&pane);
  splitter_->addWidget(&pane);
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

/*void Graph::OnFocusChanged(View* focused_before, View* focused_now) {
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
