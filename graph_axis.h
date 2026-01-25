#pragma once

#include "graph_qt/graph_cursor.h"
#include "graph_qt/model/graph_range.h"

#include <QWidget>

namespace views {

class Graph;
class GraphPlot;

class GraphAxis : public QWidget {
  Q_OBJECT

 public:
  explicit GraphAxis(QWidget* parent = nullptr);
  ~GraphAxis() override;

  void Init(Graph* graph, GraphPlot* plot, bool is_vertical);

  // The plot is null for horizontal axis.
  GraphPlot* plot() const { return plot_; }

  bool is_vertical() const { return is_vertical_; }

  // Ranges.
  const GraphRange& range() const { return range_; }
  void SetRange(const GraphRange& range);
  void UpdateRange();

  // Time fit.
  bool time_fit() const { return time_fit_; }
  void SetTimeFit(bool time_fit);
  void Fit();

  const GraphRange& scroll_range() const { return scroll_range_; }
  void SetScrollRange(const GraphRange& range);

  // Conversion.
  double ConvertScreenToValue(int pos) const;
  int ConvertValueToScreen(double value) const;

  double tick_step() const { return tick_step_; }
  void GetTickValues(double& first_value, double& last_value) const;

  using Cursors = std::vector<GraphCursor>;
  const Cursors& cursors() const { return cursors_; }
  const GraphCursor& AddCursor(double position);
  void DeleteCursor(const GraphCursor& cursor);
  void InvalidateCursor(const GraphCursor& cursor);

  // View
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void paintEvent(QPaintEvent* e) override;
  void resizeEvent(QResizeEvent* e) override;
  void contextMenuEvent(QContextMenuEvent* event) override;

 signals:
  void rangeChanged(const GraphRange& range);
  void scrollRangeChanged(const GraphRange& range);

 private:
  friend class GraphLine;

  int GetSize() const;

  QString GetLabelForValue(double value) const;

  void PaintTick(QPainter& painter, int pos) const;
  void PaintLabel(QPainter& painter, int pos, const QString& label);

  QRect GetCursorLabelRect(const GraphCursor& cursor) const;
  const GraphCursor* GetCursorLabelAt(QPoint point) const;
  void PaintCursorLabel(QPainter& painter, const GraphCursor& cursor);

  QRect GetCurrentValueRect(double value) const;
  void InvalidateCurrentValue(double value);
  void PaintCurrentValue(QPainter& painter, const GraphLine& line);

  void CalcDrawRect();

  Graph* graph_ = nullptr;
  // The `plot` is null for the horizontal axis.
  GraphPlot* plot_ = nullptr;
  bool is_vertical_ = false;

  GraphRange range_;
  GraphRange scroll_range_;

  double tick_step_ = 0.0;

  Cursors cursors_;

  bool moved_ = false;
  QPoint down_point_;  // point on button down
  QPoint last_point_;  // point on mouse move

  QRect draw_rc;

  double panning_range_max_ = std::numeric_limits<double>::max();

  bool ignore_context_menu_ = false;

  bool time_fit_ = true;
  bool time_fit_updating_ = false;
};

QString GetTimeAxisLabel(double val, double tick_step);

}  // namespace views
