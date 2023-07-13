#include "graph_qt/graph_pane.h"

#include "graph_qt/graph.h"
#include "graph_qt/graph_axis.h"
#include "graph_qt/graph_line.h"
#include "graph_qt/graph_plot.h"

namespace views {

GraphPane::GraphPane(QWidget* parent)
    : QWidget{parent}, vertical_axis_(new GraphAxis()), plot_(new GraphPlot()) {
  setFocusPolicy(Qt::ClickFocus);
}

GraphPane::~GraphPane() {}

void GraphPane::Init(Graph& graph) {
  assert(!graph_);

  graph_ = &graph;

  vertical_axis_->Init(graph_, plot_.get(), true);
  plot_->Init(graph_, this, &graph_->horizontal_axis(), vertical_axis_.get());

  vertical_axis_->setParent(this);
  plot_->setParent(this);
}

void GraphPane::mousePressEvent(QMouseEvent* e) {
  graph_->SelectPane(this);

  QWidget::mousePressEvent(e);
}

void GraphPane::resizeEvent(QResizeEvent* e) {
  int divider = std::max(0, width() - Graph::kVerticalAxisWidth);

  plot_->setGeometry(0, 0, divider, height());

  vertical_axis_->setGeometry(divider, 0, std::max(0, width() - divider),
                              height());
}

/*void GraphPane::RequestFocus() {
  plot_->RequestFocus();
}*/

}  // namespace views
