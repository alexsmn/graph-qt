#include "graph_qt/graph.h"

#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_pane.h"
#include "graph_qt/graph_plot.h"
#include "graph_qt/graph_widget.h"
#include "graph_qt/horizontal_scroll_bar_controller.h"

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <algorithm>
#include <cfloat>

namespace views {

// Graph

Graph::Graph(QWidget* parent) : QFrame{parent} {
  setLayout(new QVBoxLayout);
  layout()->setMargin(0);
  layout()->setSpacing(0);

  splitter_ = new QSplitter{this};
  splitter_->setOrientation(Qt::Vertical);
  splitter_->setHandleWidth(0);
  layout()->addWidget(splitter_);

  // The bottom layout row under the panes. It contains both the horizontal axis
  // and the scroll bar, as they are both indented from the right by the width
  // of the vertical axis.
  auto* bottom_layout = new QVBoxLayout;
  bottom_layout->setContentsMargins(0, 0, kVerticalAxisWidth, 0);
  bottom_layout->setSpacing(0);
  layout()->addItem(bottom_layout);

  horizontal_axis_ = new GraphAxis{this};
  horizontal_axis_->Init(this, nullptr, false);
  horizontal_axis_->setMinimumHeight(kHorizontalAxisHeight);
  bottom_layout->addWidget(horizontal_axis_);

  QObject::connect(horizontal_axis_, &GraphAxis::rangeChanged, this,
                   [this] { OnHorizontalAxisRangeChanged(); });
  QObject::connect(horizontal_axis_, &GraphAxis::scrollRangeChanged, this,
                   [this] {
                     horizontal_scroll_bar_controller_->SetScrollRange(
                         horizontal_axis_->scroll_range());
                   });

  auto* horizontal_scroll_bar = new QScrollBar{this};
  horizontal_scroll_bar->setOrientation(Qt::Horizontal);
  horizontal_scroll_bar->setRange(0, 0);
  horizontal_scroll_bar->setStyleSheet("background-color: none;");
  bottom_layout->addWidget(horizontal_scroll_bar);

  horizontal_scroll_bar_controller_ =
      std::make_unique<HorizontalScrollBarController>(*horizontal_scroll_bar,
                                                      *horizontal_axis_);

  setFrameStyle(QFrame::StyledPanel);
  setStyleSheet("background-color: white;");
}

Graph::~Graph() {
  DeleteAllPanes();
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
  auto i = std::find(panes_.begin(), panes_.end(), pane);
  if (i == panes_.end())
    return nullptr;
  i++;
  if (i == panes_.end())
    return nullptr;
  return *i;
}

GraphPane* Graph::GetPrevPane(GraphPane* pane) const {
  assert(pane);
  auto i = std::find(panes_.begin(), panes_.end(), pane);
  if (i == panes_.begin())
    return nullptr;
  i--;
  return *i;
}

void Graph::DeletePane(GraphPane& pane) {
  // Remove from zooming history.
  for (auto i = zooming_history_.begin(); i != zooming_history_.end(); ++i) {
    ZoomingHistoryItem& item = *i;
    if (item.pane_ == &pane)
      item.pane_ = nullptr;
  }

  if (&pane == selected_pane_)
    SelectPane(nullptr);

  // delete pane from chart
  auto pi = std::find(panes_.begin(), panes_.end(), &pane);
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
    for (auto i = panes_.begin(); i != panes_.end(); ++i)
      (*i)->plot().InvalidateCursor(cursor);
  }

  cursor.axis_->InvalidateCursor(cursor);
}

void Graph::MoveCursor(const GraphCursor& cursor, double position) {
  //::UpdateWindow(GetWindowHandle());

  InvalidateCursor(cursor);
  const_cast<GraphCursor&>(cursor).position_ = position;
  InvalidateCursor(cursor);

  if (controller_ && selected_cursor_ == &cursor) {
    controller_->OnSelectedCursorChanged();
  }
}

void Graph::SelectCursor(const GraphCursor* cursor) {
  if (selected_cursor_)
    InvalidateCursor(*selected_cursor_);

  selected_cursor_ = const_cast<GraphCursor*>(cursor);

  if (selected_cursor_) {
    InvalidateCursor(*selected_cursor_);
  }

  if (controller_) {
    controller_->OnSelectedCursorChanged();
  }
}

void Graph::DeleteCursor(const GraphCursor& cursor) {
  InvalidateCursor(cursor);

  if (selected_cursor_ == &cursor)
    selected_cursor_ = nullptr;

  cursor.axis_->DeleteCursor(cursor);

  if (controller_) {
    controller_->OnSelectedCursorChanged();
  }
}

void Graph::AdjustTimeRange(GraphRange& range) const {
  for (const auto* pane : panes_) {
    for (const auto* line : pane->plot().lines())
      line->AdjustHorizontalRange(range);
  }
}

GraphRange Graph::GetTotalHorizontalRange() const {
  GraphRange result;
  for (const auto* pane : panes_) {
    for (const auto* line : pane->plot().lines()) {
      auto range = line->GetHorizontalRange();
      if (!range.empty()) {
        assert(range.low() <= range.high());
        if (result.empty()) {
          result = range;
        } else {
          result.low_ = std::min(result.low_, range.low());
          result.high_ = std::max(result.high_, range.high());
        }
      }
    }
  }
  return result;
}

void Graph::OnHorizontalAxisRangeChanged() {
  UpdateVerticalAutoRanges();
  update();

  if (controller_) {
    controller_->OnGraphPannedHorizontally();
  }
}

void Graph::UpdateVerticalAutoRanges() {
  for (const auto* pane : panes_) {
    for (auto* line : pane->plot().lines()) {
      if (line->auto_range()) {
        line->UpdateVerticalRange();
      }
    }
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

GraphPane* Graph::AddPane() {
  auto* pane = new GraphPane{this};
  AddPane(*pane);
  return pane;
}

void Graph::AddPane(GraphPane& pane) {
  pane.Init(*this);
  pane.setParent(this);
  panes_.push_back(&pane);
  splitter_->addWidget(&pane);
}

void Graph::DeleteAllPanes() {
  for (auto i = panes_.begin(); i != panes_.end(); i++)
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

bool Graph::horizontal_scroll_bar_visible() const {
  return horizontal_scroll_bar_controller_->visible();
}

void Graph::SetHorizontalScrollBarVisible(bool visible) {
  horizontal_scroll_bar_controller_->SetVisible(visible);
}

}  // namespace views
