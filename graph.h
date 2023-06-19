#pragma once

#include "graph_qt/model/graph_range.h"

#include <QFrame>
#include <QPen>
#include <cmath>
#include <deque>
#include <list>
#include <memory>
#include <string>

class QSplitter;

namespace views {

class GraphAxis;
class GraphCursor;
class GraphLine;
class GraphPane;

class Graph : public QFrame {
 public:
  typedef std::list<GraphPane*> Panes;

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

  int vertical_cursor_label_width_;
  QPen grid_pen_;
  QColor selected_cursor_color_;

  static const int kVerticalAxisWidth = 50;
  static const int kHorizontalAxisHeight = 20;

  // Offsets from graph area.
  static const int kDrawingRectOffsetX = 10;
  static const int kDrawingRectOffsetY = 7;

 protected:
  void AdjustTimeRange(GraphRange& range);
  void AdjustTimeRange();

  // QWidget
  virtual void mousePressEvent(QMouseEvent* e) override;

 private:
  // TODO: Remove friends.
  friend class GraphAxis;
  friend class GraphLine;
  friend class GraphPane;
  friend class GraphPlot;

  struct ZoomingHistoryItem {
    GraphRange horizontal_range_;
    // If |pane_| is NULL then this pane was removed. Only horizontal range
    // needs to be restored.
    GraphPane* pane_;
    bool pane_auto_range_;
    GraphRange pane_range_;
  };

  typedef std::deque<ZoomingHistoryItem> ZoomingHistory;

  QRect GetContentsBounds() const;
  QRect GetPanesBounds() const;

  void InvalidateCursor(const GraphCursor& cursor);

  void OnHorizontalRangeUpdated();

  // FocusChangeListener
  // virtual void OnFocusChanged(View* focused_before, View* focused_now);

  QSplitter* splitter_;

  Panes panes_;

  GraphCursor* selected_cursor_;
  GraphPane* selected_pane_;

  ZoomingHistory zooming_history_;

  Controller* controller_;

  GraphAxis* horizontal_axis_;
};

}  // namespace views
