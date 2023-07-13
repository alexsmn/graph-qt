#pragma once

#include <cassert>
#include <memory>
#include <qwidget.h>

namespace views {

class Graph;
class GraphAxis;
class GraphPlot;

class GraphPane : public QWidget {
 public:
  explicit GraphPane(QWidget* parent = nullptr);
  virtual ~GraphPane();

  Graph& graph() const {
    assert(graph_);
    return *graph_;
  }

  void Init(Graph& graph);

  GraphAxis& vertical_axis() { return *vertical_axis_; }
  const GraphAxis& vertical_axis() const { return *vertical_axis_; }

  GraphPlot& plot() { return *plot_; }
  const GraphPlot& plot() const { return *plot_; }

  int size_percent_ = 100;

  // QWidget
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void resizeEvent(QResizeEvent* e) override;
  // virtual void RequestFocus() override;

 private:
  Graph* graph_ = nullptr;

  std::unique_ptr<GraphAxis> vertical_axis_;

  std::unique_ptr<GraphPlot> plot_;
};

}  // namespace views
