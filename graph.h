#pragma once

#include "graph_qt/model/graph_range.h"

#include <QFrame>
#include <QPen>
#include <cmath>
#include <deque>
#include <list>
#include <memory>
#include <string>

class QScrollBar;
class QSplitter;

namespace views {

class GraphAxis;
class GraphCursor;
class GraphLine;
class GraphPane;

class Graph : public QFrame {
 public:
  using Panes = std::list<GraphPane*>;

  class Controller {
   public:
    virtual void OnGraphModified() {}
    virtual void OnGraphSelectPane() {}
    virtual void OnGraphPannedHorizontally() {}
    virtual void OnLineItemChanged(GraphLine& line) {}
  };

  Graph();
  virtual ~Graph();

  Controller* controller() const { return controller_; }
  void set_controller(Controller* controller) { controller_ = controller; }

  const Panes& panes() const { return panes_; }

  GraphPane* selected_pane() const { return selected_pane_; }
  void SelectPane(GraphPane* pane);

  GraphAxis& horizontal_axis() { return *horizontal_axis_; }
  const GraphAxis& horizontal_axis() const { return *horizontal_axis_; }

  QScrollBar& horizontal_scroll_bar() { return *horizontal_scroll_bar_; }
  const QScrollBar& horizontal_scroll_bar() const {
    return *horizontal_scroll_bar_;
  }
  void setHorizontalScrollMin(double value);

  void UpdateAutoRanges();
  void Zoom(GraphPane& pane,
            const GraphRange& horizontal_range,
            const GraphRange& vertical_range);

  // Panes.
  void AddPane(GraphPane& pane);
  void DeletePane(GraphPane& pane);
  void DeleteAllPanes();
  GraphPane* GetNextPane(GraphPane* pane) const;
  GraphPane* GetPrevPane(GraphPane* pane) const;

  // Cursors.
  GraphCursor* selected_cursor() const { return selected_cursor_; }
  void MoveCursor(const GraphCursor& cursor, double position);
  void SelectCursor(const GraphCursor* cursor);
  void DeleteCursor(const GraphCursor& cursor);

  virtual void UpdateCurBox() {}
  virtual QString GetCursorLabel(const GraphCursor& cursor) const;

  // Get horizontal label string. Shall be called only in scope of DrawYAxis().
  virtual QString GetXAxisLabel(double value) const;

  // View
  virtual void resizeEvent(QResizeEvent* e) override;
  /*virtual bool IsFocusable() const { return true; }
  virtual void RequestFocus();*/

  int vertical_cursor_label_width_ = 70;
  QPen grid_pen_{QColor(237, 237, 237)};
  QColor selected_cursor_color_{100, 100, 100};

  static const int kVerticalAxisWidth = 50;
  static const int kHorizontalAxisHeight = 20;
  static const int kHorizontalScrollBarHeight = 20;

  // Offsets from graph area.
  static const int kDrawingRectOffsetX = 10;
  static const int kDrawingRectOffsetY = 7;

 protected:
  void AdjustTimeRange(GraphRange& range);
  void AdjustTimeRange();

  // QWidget
  virtual void mousePressEvent(QMouseEvent* e) override;

 private:
  struct ZoomingHistoryItem {
    GraphRange horizontal_range_;
    // If |pane_| is NULL then this pane was removed. Only horizontal range
    // needs to be restored.
    GraphPane* pane_;
    bool pane_auto_range_;
    GraphRange pane_range_;
  };

  using ZoomingHistory = std::deque<ZoomingHistoryItem>;

  QRect GetContentsBounds() const;
  QRect GetPanesBounds() const;

  void InvalidateCursor(const GraphCursor& cursor);

  void OnHorizontalRangeUpdated();

  void UpdateHorizontalScrollRange();

  // FocusChangeListener
  // virtual void OnFocusChanged(View* focused_before, View* focused_now);

  QSplitter* splitter_ = nullptr;

  Panes panes_;

  GraphCursor* selected_cursor_ = nullptr;
  GraphPane* selected_pane_ = nullptr;

  ZoomingHistory zooming_history_;

  Controller* controller_ = nullptr;

  GraphAxis* horizontal_axis_ = nullptr;

  QScrollBar* horizontal_scroll_bar_ = nullptr;
  double horizontal_scroll_min_ = 0.0;

  // TODO: Remove friends.
  friend class GraphAxis;
  friend class GraphLine;
  friend class GraphPane;
  friend class GraphPlot;
};

}  // namespace views
