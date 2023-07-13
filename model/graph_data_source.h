#pragma once

#include "graph_qt/model/graph_range.h"

#include <QString>

namespace views {

struct GraphPoint;

class PointEnumerator {
 public:
  virtual ~PointEnumerator() = default;

  virtual size_t GetCount() const = 0;

  virtual bool EnumNext(GraphPoint& value) = 0;
};

class GraphDataSource {
 public:
  class Observer {
   public:
    virtual void OnDataSourceHistoryChanged() {}
    virtual void OnDataSourceCurrentValueChanged() {}
    virtual void OnDataSourceItemChanged() {}
    virtual void OnDataSourceDeleted() {}
  };

  GraphDataSource();
  virtual ~GraphDataSource();

  void SetObserver(Observer* observer) { observer_ = observer; }

  virtual double GetCurrentValue() const { return kGraphUnknownValue; };

  virtual PointEnumerator* EnumPoints(double from,
                                      double to,
                                      bool include_left_bound,
                                      bool include_right_bound) = 0;

  virtual QString GetYAxisLabel(double value) const;

  // Must be O(1).
  virtual GraphRange GetHorizontalRange() const { return GraphRange{}; }

  // Must be O(1).
  virtual GraphRange GetVerticalRange() const { return GraphRange{}; }

  GraphRange CalculateAutoRange(double x1, double x2);

  // Limits.
  double limit_lo_ = kGraphUnknownValue;
  double limit_hi_ = kGraphUnknownValue;
  double limit_lolo_ = kGraphUnknownValue;
  double limit_hihi_ = kGraphUnknownValue;

 protected:
  Observer* observer_ = nullptr;

 private:
  double current_value_ = kGraphUnknownValue;
};

}  // namespace views
