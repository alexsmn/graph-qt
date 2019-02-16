#pragma once

#include <qwidget.h>

#include "graph_qt/graph_cursor.h"
#include "graph_qt/model/graph_range.h"

namespace views {

class Graph;
class GraphPlot;

class GraphAxis : public QWidget {
 public:
  GraphAxis();
  ~GraphAxis();

  void Init(Graph* graph, GraphPlot* plot, bool is_vertical);

  // |plot()| is NULL for horizontal axis.
  GraphPlot* plot() const { return plot_; }
  bool is_vertical() const { return is_vertical_; }

  // Ranges.
  const GraphRange& range() const { return range_; }
  void SetRange(const GraphRange& range);
  void UpdateRange();

  double panning_range_max() const { return panning_range_max_; }
  void SetPanningRangeMax(double value) { panning_range_max_ = value; }

  // Conversion.
  double ConvertScreenToValue(int pos) const;
  int ConvertValueToScreen(double value) const;

  double tick_step() const { return tick_step_; }
  void GetTickValues(double& first_value, double& last_value) const;

  typedef std::vector<GraphCursor> Cursors;
  const Cursors& cursors() const { return cursors_; }
  const GraphCursor& AddCursor(double position);
  void DeleteCursor(const GraphCursor& cursor);
  void InvalidateCursor(const GraphCursor& cursor);

  // View
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void resizeEvent(QResizeEvent* e) override;

 private:
  friend class GraphLine;

  int GetSize() const;

  QString GetLabelForValue(double value) const;

  void PaintTick(QPainter& painter, int pos);
  void PaintLabel(QPainter& painter, int pos, const QString& label);

  QRect GetCursorLabelRect(const GraphCursor& cursor) const;
  const GraphCursor* GetCursorLabelAt(QPoint point) const;
  void PaintCursorLabel(QPainter& painter, const GraphCursor& cursor);

  QRect GetCurrentValueRect(double value) const;
  void InvalidateCurrentValue(double value);
  void PaintCurrentValue(QPainter& painter, const GraphLine& line);

  void CalcDrawRect();

  Graph* graph_;
  GraphPlot* plot_;
  bool is_vertical_;

  GraphRange range_;

  double tick_step_;

  Cursors cursors_;

  bool moved_;
  QPoint down_point_;  // point on button down
  QPoint last_point_;  // point on mouse move

  QRect draw_rc;

  double panning_range_max_;
};

}  // namespace views
