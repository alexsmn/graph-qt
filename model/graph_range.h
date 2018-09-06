#pragma once

#include "base/logging.h"

#include <cassert>
#include <float.h>
#include <math.h>

namespace views {

const double kGraphUnknownValue = FLT_MIN;

class GraphRange {
 public:
  enum Kind { LINEAR, LOGICAL, TIME };

  GraphRange()
      : low_(kGraphUnknownValue),
        high_(kGraphUnknownValue),
        kind_(LINEAR) {
  }
  GraphRange(double low, double high)
      : low_(low),
        high_(high),
        kind_(LINEAR) {
    DCHECK_LE(low, high);
  }
  GraphRange(double low, double high, Kind kind)
      : low_(low),
        high_(high),
        kind_(kind) {
    DCHECK_LE(low, high);
  }

  double low() const { return low_; }
  double high() const { return high_; }
  double delta() const { assert(low_ <= high_); return high_ - low_; }

  bool empty() const { return abs(delta()) < FLT_EPSILON; }

  Kind kind() const { return kind_; }
  void set_time() { kind_ = TIME; }

  bool Contains(double value) const {
    DCHECK_LE(low_, high_);
    return (value >= low_) && (value <= high_);
  }
  
  void Offset(double delta) {
    low_ += delta;
    high_ += delta;
    if (kind_ == LOGICAL)
      kind_ = LINEAR;
  }
  
  bool operator==(const GraphRange& other) const {
    return low_ == other.low_ && high_ == other.high_ &&
           kind_ == other.kind_;
  }
  bool operator!=(const GraphRange& other) const {
    return !operator==(other);
  }

  static GraphRange Logical() {
    return GraphRange(0.0, 1.0, LOGICAL);
  }

 private: 
  double low_;
  double high_;
  Kind kind_;
};

} // namespace views
