#pragma once

#include <qwidget.h>

namespace views {

class Graph;
class GraphPane;
class GraphPlot;

class GraphWidget : public QWidget {
 public:
  explicit GraphWidget(GraphPane& pane);

  Graph& graph() const;
  GraphPane& pane() const { return pane_; }
  GraphPlot& plot() const;

  void Move(const QPoint& delta);

  // View
  //virtual gfx::Size GetPreferredSize() const { return gfx::Size(); }
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;
  virtual void mouseMoveEvent(QMouseEvent* e) override;

protected:
  GraphPane& pane_;

  bool dragging_;
  QPoint last_point_;
};

} // namespace views
