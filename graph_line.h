#pragma once

#include "graph_qt/model/graph_data_source.h"
#include "graph_qt/model/graph_range.h"

#include <QColor>
#include <QString>
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
  void SetColor(const QColor& color);

  int line_weight() const { return line_weight_; }
  void SetLineWeight(int line_weight);

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

  // The four analog-limit bands, low → high.
  enum class LimitBand { kLoLo, kLo, kHi, kHiHi };

  // Optional presentation for one limit band. When `color` is invalid the band
  // is drawn in the series colour (the historical look); when `label` is
  // non-empty it is captioned beside the line. Callers (e.g. the SCADA client
  // under its opt-in severity theme) set these to colour alarm/warning bands.
  struct LimitStyle {
    QColor color;   // invalid => series colour
    QString label;  // empty => no caption
  };

  // Sets / clears the presentation of a limit band. Repaints the plot.
  void SetLimitStyle(LimitBand band, const LimitStyle& style);
  void ClearLimitStyles();

  void DrawLimit(QPainter& painter,
                 const QRect& rect,
                 double limit,
                 const LimitStyle& style) const;

  virtual void Draw(QPainter& painter, const QRect& rect);

 protected:
  // GraphDataSource::Observer
  void OnDataSourceItemChanged() override;
  void OnDataSourceHistoryChanged() override;
  void OnDataSourceCurrentValueChanged() override;

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
    if (set) {
      flags_ |= flag;
    } else {
      flags_ &= ~flag;
    }
  }

  void SetCurrentValue(double value);

  GraphPlot* plot_ = nullptr;

  GraphDataSource* data_source_ = nullptr;

  GraphRange vertical_range_;

  double current_value_ = kGraphUnknownValue;

  QColor color_ = Qt::black;

  // Per-band limit presentation, indexed by LimitBand. Default-constructed
  // (invalid colour, empty label) => the historical series-coloured line.
  LimitStyle limit_styles_[4];

  unsigned flags_ = STEPPED | AUTO_RANGE | SHOW_DOTS;
  int line_weight_ = 1;

  friend class Graph;
  friend class GraphPlot;
};

}  // namespace views
