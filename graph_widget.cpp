#include "graph_qt/graph_widget.h"

#include <qevent.h>

#include "graph_qt/graph.h"
#include "graph_qt/graph_pane.h"

namespace views {

GraphWidget::GraphWidget(GraphPane& pane)
    : pane_(pane),
      dragging_(false) {
}

Graph& GraphWidget::graph() const {
  return pane().graph();
}

GraphPlot& GraphWidget::plot() const {
  return pane().plot();
}

void GraphWidget::Move(const QPoint& delta) {
  QRect rc = geometry();
  int left = rc.x();
  int top = rc.y();

  left += delta.x();
  top += delta.y();

  if (left + width() >= pane_.width()) {
    left = pane_.width() - width();
  }
  if (top + height() >= pane_.height()) {
    top = pane_.height() - height();
  }

  if (left < 0) {
    left = 0;
  }
  if (top < 0) {
    top = 0;
  }

  setGeometry(left, top, width(), height());
}

void GraphWidget::mousePressEvent(QMouseEvent* e) {
  if (!(e->buttons() & Qt::LeftButton)) {
    return;
  }

  last_point_ = e->globalPos();
  e->accept();
  dragging_ = true;
}

void GraphWidget::mouseReleaseEvent(QMouseEvent* e) {
  dragging_ = false;
  e->accept();
}

void GraphWidget::mouseMoveEvent(QMouseEvent* e) {
  if (dragging_) {
    auto pos = e->globalPos();
    Move(pos - last_point_);
    last_point_ = pos;
    e->accept();
  }
}

} // namespace views
