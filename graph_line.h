#pragma once

#include "graph_qt/model/graph_data_source.h"
#include "graph_qt/model/graph_range.h"

#include <QColor>
#include <cassert>

class QPainter;
class QPen;
class QPoint;
class QRect;

namespace views {

class GraphPlot;
struct GraphPoint;

class GraphLine : protected GraphDataSource::Observer {
 public:
  GraphLine();
  virtual ~GraphLine();

  QColor color() const { return color_; }
  void SetColor(QColor color);

  GraphDataSource* data_source() { return data_source_; }
  const GraphDataSource* data_source() const { return data_source_; }
  void SetDataSource(GraphDataSource* data_source);

  GraphPlot& plot() const {
    assert(plot_);
    return *plot_;
  }

  bool stepped() const { return (flags_ & STEPPED) != 0; }
  bool auto_range() const { return (flags_ & AUTO_RANGE) != 0; }
  bool dots_shown() const { return (flags_ & SHOW_DOTS) != 0; }
  bool smooth() const { return (flags_ & SMOOTH) != 0; }

  void set_auto_range(bool auto_range) { set_flag(AUTO_RANGE, auto_range); }
  void set_dots_shown(bool shown) { set_flag(SHOW_DOTS, shown); }
  void set_stepped(bool stepped) { set_flag(STEPPED, stepped); }
  void set_smooth(bool smooth) { set_flag(SMOOTH, smooth); }

  const GraphRange& vertical_range() const { return vertical_range_; }
  void SetVerticalRange(const GraphRange& range);

  GraphRange GetHorizontalRange() const;

  double current_value() const { return current_value_; }

  // Shrinks the horizontal range by advancing the `low_` bound, so only the
  // `kMaxPoints` amount of points is displayed.
  void AdjustHorizontalRange(GraphRange& range) const;

  double XToValue(int x) const;
  double YToValue(int y) const;
  int ValueToX(double value) const;
  int ValueToY(double value) const;

  bool GetNearestPoint(const QPoint& screen_point,
                       GraphPoint& data_point,
                       int max_distance);

  void DrawLimit(QPainter& painter,
                 const QRect& rect,
                 double limit,
                 const QPen& pen);

  virtual void Draw(QPainter& painter, const QRect& rect);

 protected:
  // GraphDataSource::Observer
  virtual void OnDataSourceItemChanged() override;
  virtual void OnDataSourceHistoryChanged() override;
  virtual void OnDataSourceCurrentValueChanged() override;

 private:
  // graph line flags_
  enum {
    STEPPED = 0x0001,
    AUTO_RANGE = 0x0002,
    SHOW_DOTS = 0x0004,
    SMOOTH = 0x0008,
  };

  void UpdateHorizontalRange();

  void UpdateVerticalRange();
  GraphRange CalculateVerticalAutoRange();
  void SetVerticalRangeHelper(const GraphRange& range);

  void set_flag(int flag, bool set) {
    if (set)
      flags_ |= flag;
    else
      flags_ &= ~flag;
  }

  void SetCurrentValue(double value);

  GraphPlot* plot_ = nullptr;

  GraphDataSource* data_source_ = nullptr;

  GraphRange vertical_range_;

  double current_value_ = kGraphUnknownValue;

  QColor color_ = Qt::black;

  unsigned flags_ = STEPPED | AUTO_RANGE | SHOW_DOTS;
  int line_weight_ = 1;

  friend class Graph;
  friend class GraphPlot;
};

}  // namespace views
