#pragma once

#include <cassert>
#include <limits>
#include <ostream>

namespace views {

const double kGraphUnknownValue = std::numeric_limits<double>::min();

class GraphRange {
 public:
  enum Kind { LINEAR, LOGICAL, TIME };

  GraphRange() = default;

  GraphRange(double low, double high, Kind kind = LINEAR)
      : low_{low}, high_{high}, kind_{kind} {
    assert(low_ <= high_);
  }

  double low() const { return low_; }
  double high() const { return high_; }
  double delta() const {
    assert(low_ <= high_);
    return high_ - low_;
  }

  bool empty() const {
    return fabs(delta()) < std::numeric_limits<double>::epsilon();
  }

  Kind kind() const { return kind_; }
  void set_time() { kind_ = TIME; }

  bool Contains(double value) const {
    assert(low_ <= high_);
    return (value >= low_) && (value <= high_);
  }

  void Offset(double delta) {
    low_ += delta;
    high_ += delta;
    if (kind_ == LOGICAL)
      kind_ = LINEAR;
  }

  bool operator==(const GraphRange& other) const = default;

  static GraphRange Logical() { return GraphRange(0.0, 1.0, LOGICAL); }

  double low_ = kGraphUnknownValue;
  double high_ = kGraphUnknownValue;
  Kind kind_ = LINEAR;
};

namespace detail {

inline void PrintBound(std::ostream& os, double bound) {
  if (bound == kGraphUnknownValue)
    os << "unknown";
  else
    os << bound;
}

}  // namespace detail

inline std::ostream& operator<<(std::ostream& os, const GraphRange& range) {
  os << "{low=";
  detail::PrintBound(os, range.low());
  os << ", high=";
  detail::PrintBound(os, range.high());
  os << ", delta=" << range.delta()
     << ", kind=" << static_cast<int>(range.kind()) << "}";
  return os;
}

}  // namespace views
