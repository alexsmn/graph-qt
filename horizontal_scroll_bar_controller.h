#pragma once

#include "graph_qt/model/graph_range.h"

class QScrollBar;

namespace views {

class GraphAxis;

class HorizontalScrollBarController {
 public:
  HorizontalScrollBarController(QScrollBar& scroll_bar, GraphAxis& axis);

  bool visible() const { return visible_; }
  void SetVisible(bool visible);

  void SetScrollRange(const GraphRange& range);

 private:
  void OnViewRangeChanged(const GraphRange& view_range);
  void OnScroll(int pos);

  QScrollBar& scroll_bar_;
  GraphAxis& axis_;

  // Have to track scroll visibility explicitly, as `isVisible` gives false when
  // the `Graph` widget is detached.
  bool visible_ = false;

  GraphRange scroll_range_;
  double scroll_step_ = 0.0;
  bool updating_ = false;
};

}  // namespace views
