#pragma once

#include <cassert>
#include <qcolor.h>

#include "graph_qt/model/graph_data_source.h"
#include "graph_qt/model/graph_range.h"

class QPainter;
class QPen;
class QPoint;
class QRect;

namespace views {

class GraphPlot;
struct GraphPoint;

class GraphLine : protected GraphDataSource::Observer {
 public:
  // graph line flags
  enum {
    STEPPED    = 0x0001,
    AUTO_RANGE = 0x0002,
    SHOW_DOTS  = 0x0004,
    SMOOTH     = 0x0008,
  };
  
  GraphLine();
  virtual ~GraphLine();

  GraphDataSource* data_source() { return data_source_; }
  const GraphDataSource* data_source() const { return data_source_; }
  void SetDataSource(GraphDataSource* data_source);

  GraphPlot& plot() const { assert(plot_); return *plot_; }

  bool stepped() const { return (flags & STEPPED) != 0; }
  bool auto_range() const { return (flags & AUTO_RANGE) != 0; }
  bool dots_shown() const { return (flags & SHOW_DOTS) != 0; }
  bool smooth() const { return (flags & SMOOTH) != 0; }
  
  void set_auto_range(bool auto_range) { set_flag(AUTO_RANGE, auto_range); }
  void set_dots_shown(bool shown) { set_flag(SHOW_DOTS, shown); }
  void set_stepped(bool stepped) { set_flag(STEPPED, stepped); }
  void set_smooth(bool smooth) { set_flag(SMOOTH, smooth); }
  
  const GraphRange& range() const { return range_; }
  void SetRange(const GraphRange& range);

  double current_value() const { return current_value_; }

  void UpdateAutoRange();
  
  double XToValue(int x) const;
  double YToValue(int y) const;
  int ValueToX(double value) const;
  int ValueToY(double value) const;

  bool GetNearestPoint(const QPoint& screen_point, GraphPoint& data_point, int max_distance);

  void DrawLimit(QPainter& painter, const QRect& rect, double limit, const QPen& pen);

  virtual void Draw(QPainter& painter, const QRect& rect);

  unsigned flags;
  QColor color;
  int line_weight_;

 protected:
  // GraphDataSource::Observer
  virtual void OnDataSourceItemChanged() override;
  virtual void OnDataSourceHistoryChanged() override;
  virtual void OnDataSourceCurrentValueChanged() override;

 private:
  friend class Graph;
  friend class GraphPlot;
 
  void UpdateRange();
  GraphRange CalculateAutoRange();
  void SetRangeHelper(const GraphRange& range);

  void set_flag(int flag, bool set) {
    if (set)
      flags |= flag;
    else
      flags &= ~flag;
  }

  void SetCurrentValue(double value);

  GraphPlot* plot_;

  GraphDataSource* data_source_;

  GraphRange range_;

  double current_value_;

  DISALLOW_COPY_AND_ASSIGN(GraphLine);
};

} // namespace views
