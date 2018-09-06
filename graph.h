#pragma once

#include "graph_qt/model/graph_range.h"

#include <QFrame>
#include <QPen>
#include <cmath>
#include <deque>
#include <list>
#include <memory>
#include <string>

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

  void Fit();
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

  // View
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void resizeEvent(QResizeEvent* e) override;
  /*virtual View* GetEventHandlerForPoint(const gfx::Point& point);
  virtual gfx::NativeCursor GetCursor(const gfx::Point& point) const;
  virtual bool IsFocusable() const { return true; }
  virtual void RequestFocus();*/

  double right_range_limit_;
  int vertical_cursor_label_width_;
  bool m_time_fit;
  QPen grid_pen_;
  QPen selected_cursor_pen_;
  QColor selected_cursor_color_;

  static const int kVerticalAxisWidth = 50;
  static const int kHorizontalAxisHeight = 20;

  // Offsets from graph area.
  static const int kDrawingRectOffsetX = 10;
  static const int kDrawingRectOffsetY = 7;

 protected:
  // View
  /*virtual bool OnMousePressed(const ui::MouseEvent& event) override;
  virtual bool OnMouseDragged(const ui::MouseEvent& event) override;
  virtual void OnMouseReleased(const ui::MouseEvent& event) override;
  virtual void ViewHierarchyChanged(const ViewHierarchyChangedDetails& details)
  override;*/

 private:
  // TODO: Remove friends.
  friend class GraphAxis;
  friend class GraphLine;
  friend class GraphPane;
  friend class GraphPlot;

  enum State {
    STATE_MOUSE_DOWN,
    STATE_IDLE,
    // TODO: Move to plot.
    STATE_PANE_RESIZING,
  };

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

  GraphPane* GetPaneSizerAt(QPoint point) const;

  void InvalidateCursor(const GraphCursor& cursor);

  void OnHorizontalRangeUpdated();

  void AdjustTimeRange();
  void AdjustTimeRange(GraphRange& range);

  // Get horizontal label string. Shall be called only in scope of DrawYAxis().
  virtual QString GetXAxisLabel(double value) const;

  // FocusChangeListener
  // virtual void OnFocusChanged(View* focused_before, View* focused_now);

  Panes panes_;

  GraphCursor* selected_cursor_;
  GraphPane* selected_pane_;

  State state_;
  QPoint down_point_;  // point on button down
  QPoint last_point_;  // point on mouse move
  GraphPane* resizing_pane_;

  ZoomingHistory zooming_history_;

  Controller* controller_;

  GraphAxis* horizontal_axis_;
};

}  // namespace views
