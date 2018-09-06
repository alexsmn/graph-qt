#pragma once

#include "graph_qt/model/graph_range.h"

#include <QString>

namespace views {

struct GraphPoint;

class PointEnumerator {
 public:
  virtual ~PointEnumerator() {}

  virtual size_t GetCount() const = 0;

  virtual bool EnumNext(GraphPoint& value) = 0;
};

class GraphDataSource {
 public:
  class Observer {
   public:
    virtual ~Observer() {}

    // History changed.
    virtual void OnDataSourceHistoryChanged() {}
    virtual void OnDataSourceCurrentValueChanged() {}
    virtual void OnDataSourceItemChanged() {}
    virtual void OnDataSourceDeleted() {}
  };

  GraphDataSource();
  virtual ~GraphDataSource();

  void SetObserver(Observer* observer) { observer_ = observer; }

  const GraphRange& range() const { return range_; }

  double current_value() const { return current_value_; }
  void SetCurrentValue(double value);

  virtual PointEnumerator* EnumPoints(double from, double to,
                                      bool include_left_bound,
                                      bool include_right_bound) = 0;

  virtual QString GetYAxisLabel(double value) const;

  GraphRange CalculateAutoRange(double x1, double x2);

  GraphRange range_;

  // Limits.
  double limit_lo_;
  double limit_hi_;
  double limit_lolo_;
  double limit_hihi_;

 protected:
  Observer* observer_;

 private:
  double current_value_;
};

} // namespace views
