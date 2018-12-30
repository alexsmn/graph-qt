#pragma once

#include "graph_qt/model/graph_range.h"
#include "graph_qt/model/graph_types.h"

#include <QWidget>
#include <list>

namespace views {

class Graph;
class GraphAxis;
class GraphCursor;
class GraphLine;
class GraphPane;
class GraphWidget;

class GraphPlot : public QWidget {
 public:
  GraphPlot();
  virtual ~GraphPlot();

  void Init(Graph* graph,
            GraphPane* pane,
            GraphAxis* horizontal_axis,
            GraphAxis* vertical_axis);

  Graph& graph() const {
    assert(graph_);
    return *graph_;
  }
  GraphPane& pane() const {
    assert(pane_);
    return *pane_;
  }

  // Axises.
  GraphAxis& horizontal_axis() {
    assert(horizontal_axis_);
    return *horizontal_axis_;
  }
  GraphAxis& vertical_axis() {
    assert(vertical_axis_);
    return *vertical_axis_;
  }

  // Lines.
  typedef std::list<GraphLine*> Lines;
  const Lines& lines() const { return lines_; }
  GraphLine* primary_line() const {
    return lines_.empty() ? NULL : lines_.front();
  }
  void AddLine(GraphLine& line);
  void DeleteLine(GraphLine& line);
  void DeleteAllLines();

  // Zooming.
  bool zooming() const { return zooming_; }
  void set_zooming(bool zooming) { zooming_ = zooming; }

  // Cursors.
  typedef std::vector<GraphCursor> Cursors;
  void InvalidateCursor(const GraphCursor& cursor);

  // Focusing.
  void SetFocusPoint(const GraphPoint& point, GraphLine* line);
  void InvalidateFocusPoint();

  // Widgets.
  typedef std::list<GraphWidget*> Widgets;
  const Widgets& widgets() const { return widgets_; }
  void AddWidget(GraphWidget& widget);
  void RemoveWidget(GraphWidget& widget);

  // View
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;
  virtual void leaveEvent(QEvent* e) override;
  /*virtual bool IsFocusable() const override { return true; }
  virtual bool GetTooltipText(const gfx::Point& point,
                              base::string16* tooltip) const override;*/

 protected:
  // QWidget
  bool event(QEvent* event) override;

 private:
  enum State { STATE_MOUSE_DOWN, STATE_IDLE, STATE_PANNING, STATE_ZOOMING };

  void PaintHorizontalGrid(QPainter& painter);
  void PaintVerticalGrid(QPainter& painter);
  void PaintZoomRect(QPainter& painter);
  void PaintCursor(QPainter& painter, const GraphCursor& cursor);

  Graph* graph_;
  GraphPane* pane_;
  GraphAxis* horizontal_axis_;
  GraphAxis* vertical_axis_;

  Lines lines_;

  Widgets widgets_;

  State state_;
  QPoint down_point_;  // point on button down
  QPoint last_point_;  // point on mouse move

  bool zooming_;

  GraphLine* focus_line_;
  GraphPoint focus_point_;
  QString focus_tooltip_;
};

}  // namespace views
