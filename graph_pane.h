#pragma once

#include <qwidget.h>
#include <cassert>
#include <memory>

namespace views {

class Graph;
class GraphAxis;
class GraphPlot;

class GraphPane : public QWidget {
 public:
  GraphPane();
  virtual ~GraphPane();

  Graph& graph() const {
    assert(graph_);
    return *graph_;
  }
  void Init(Graph& graph);

  GraphAxis& vertical_axis() const { return *vertical_axis_; }
  GraphPlot& plot() const { return *plot_; }

  int size_percent_;

  // QWidget
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void resizeEvent(QResizeEvent* e) override;
  // virtual void RequestFocus() override;

 private:
  Graph* graph_;

  std::unique_ptr<GraphAxis> vertical_axis_;

  std::unique_ptr<GraphPlot> plot_;
};

}  // namespace views
