#pragma once

namespace views {

class GraphAxis;

class GraphCursor {
 public:
  GraphCursor(double position, GraphAxis* axis)
      : position_(position),
        axis_(axis) {
  }

  double position_;
  GraphAxis* axis_;
};

} // namespace views
