#pragma once

#include "base/time/time.h"
#include "graph_qt/model/graph_types.h"

#include <chrono>

namespace views {

class GraphTimeHelper {
 public:
  inline static const double sec = 1.0;
  inline static const double min = 60 * sec;
  inline static const double hour = 60 * min;
  inline static const double day = 24 * hour;
  inline static const double msec = sec / 1000;

 private:
  GraphTimeHelper();
};

inline GraphValue ValueFromTime(base::Time time) {
  return time.ToDoubleT();
}

inline GraphValue ValueFromDuration(base::TimeDelta duration) {
  return duration.InSecondsF();
}

}  // namespace views