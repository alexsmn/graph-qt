#pragma once

#include "graph_qt/model/graph_types.h"

#include <QDateTime>
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

// Converts QDateTime to graph value (Unix timestamp in seconds).
inline GraphValue ValueFromTime(const QDateTime& time) {
  return time.toMSecsSinceEpoch() / 1000.0;
}

// Converts std::chrono::duration to graph value (seconds).
template <typename Rep, typename Period>
inline GraphValue ValueFromDuration(std::chrono::duration<Rep, Period> duration) {
  return std::chrono::duration<double>(duration).count();
}

}  // namespace views