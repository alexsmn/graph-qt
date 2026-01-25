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
class HorizontalScrollBarController;

class Graph : public QFrame {
 public:
  using Panes = std::list<GraphPane*>;

  class Controller {
   public:
    virtual void OnGraphModified() {}
    virtual void OnGraphSelectPane() {}
    virtual void OnGraphPannedHorizontally() {}
    virtual void OnLineItemChanged(GraphLine& line) {}
    virtual void OnSelectedCursorChanged() {}
  };

  explicit Graph(QWidget* parent = nullptr);
  ~Graph() override;

  Controller* controller() const { return controller_; }
  void set_controller(Controller* controller) { controller_ = controller; }

  const Panes& panes() const { return panes_; }

  GraphPane* selected_pane() const { return selected_pane_; }
  void SelectPane(GraphPane* pane);

  GraphAxis& horizontal_axis() { return *horizontal_axis_; }
  const GraphAxis& horizontal_axis() const { return *horizontal_axis_; }

  bool horizontal_scroll_bar_visible() const;
  void SetHorizontalScrollBarVisible(bool visible);

  void Zoom(GraphPane& pane,
            const GraphRange& horizontal_range,
            const GraphRange& vertical_range);

  // Panes.
  GraphPane* AddPane();
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

  virtual QString GetCursorLabel(const GraphCursor& cursor) const;

  // Get horizontal label string. Shall be called only in scope of DrawYAxis().
  virtual QString GetXAxisLabel(double value) const;

  // View
  /*virtual bool IsFocusable() const { return true; }
  virtual void RequestFocus();*/

  int vertical_cursor_label_width_ = 70;
  QPen grid_pen_{QColor(237, 237, 237)};
  QColor selected_cursor_color_{100, 100, 100};

  static const int kVerticalAxisWidth = 50;
  static const int kHorizontalAxisHeight = 22;
  static const int kHorizontalScrollBarHeight = 20;

  // Offsets from graph area.
  static const int kDrawingRectOffsetX = 10;
  static const int kDrawingRectOffsetY = 7;

 protected:
  void AdjustTimeRange(GraphRange& range) const;

  // QWidget
  void mousePressEvent(QMouseEvent* e) override;

 private:
  struct ZoomingHistoryItem {
    GraphRange horizontal_range_;
    // If |pane_| is NULL then this pane was removed. Only horizontal range
    // needs to be restored.
    GraphPane* pane_ = nullptr;
    bool pane_auto_range_ = false;
    GraphRange pane_range_;
  };

  using ZoomingHistory = std::deque<ZoomingHistoryItem>;

  void InvalidateCursor(const GraphCursor& cursor);

  void OnHorizontalAxisRangeChanged();
  GraphRange GetTotalHorizontalRange() const;

  void UpdateVerticalAutoRanges();

  // FocusChangeListener
  // virtual void OnFocusChanged(View* focused_before, View* focused_now);

  QSplitter* splitter_ = nullptr;

  Panes panes_;

  GraphCursor* selected_cursor_ = nullptr;
  GraphPane* selected_pane_ = nullptr;

  ZoomingHistory zooming_history_;

  Controller* controller_ = nullptr;

  GraphAxis* horizontal_axis_ = nullptr;

  std::unique_ptr<HorizontalScrollBarController>
      horizontal_scroll_bar_controller_;

  // TODO: Remove friends.
  friend class GraphAxis;
  friend class GraphLine;
  friend class GraphPane;
  friend class GraphPlot;
};

}  // namespace views
