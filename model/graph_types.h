#pragma once

namespace views {

struct GraphPoint {
  GraphPoint() : x(0.0), y(0.0), good(false) {}
  GraphPoint(double x, double y) : x(x), y(y), good(false) {}

  bool operator==(const GraphPoint& other) const {
    return x == other.x && y == other.y && good == other.good;
  }

  double x;
  double y;
  bool good;
};

} // namespace views
