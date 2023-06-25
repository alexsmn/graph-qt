#pragma once

#include "graph_qt/model/graph_range.h"

class QScrollBar;

namespace views {

class GraphAxis;

class HorizontalScrollBarController {
 public:
  HorizontalScrollBarController(QScrollBar& scroll_bar, GraphAxis& axis);

  void SetScrollRange(const GraphRange& range);

 private:
  void OnViewRangeChanged(const GraphRange& view_range);
  void OnScroll(int pos);

  QScrollBar& scroll_bar_;
  GraphAxis& axis_;

  GraphRange scroll_range_;
  double scroll_step_ = 0.0;
  bool updating_ = false;
};

}  // namespace views
