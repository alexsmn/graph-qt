#include "graph_qt/horizontal_scroll_bar_controller.h"

#include "base/auto_reset.h"
#include "graph_qt/graph_axis.h"

#include <QScrollBar>

namespace views {

namespace {

// Advance on 10% of the view range on each scroll.
const int kScrollPageStep = 10;

}  // namespace

HorizontalScrollBarController::HorizontalScrollBarController(
    QScrollBar& scroll_bar,
    GraphAxis& horizontal_axis)
    : scroll_bar_{scroll_bar}, axis_{horizontal_axis} {
  scroll_bar_.setPageStep(kScrollPageStep);

  QObject::connect(
      &axis_, &GraphAxis::rangeChanged,
      std::bind_front(&HorizontalScrollBarController::OnViewRangeChanged,
                      this));

  QObject::connect(
      &scroll_bar_, &QScrollBar::valueChanged,
      std::bind_front(&HorizontalScrollBarController::OnScroll, this));
}

void HorizontalScrollBarController::SetScrollRange(const GraphRange& range) {
  scroll_range_ = range;

  base::AutoReset updating{&updating_, true};

  auto view_range = axis_.range();
  if (view_range.empty() || scroll_range_.empty()) {
    scroll_bar_.setRange(0, 0);
    return;
  }

  scroll_step_ = view_range.delta() / kScrollPageStep;
  int count = static_cast<int>((scroll_range_.delta() - view_range.delta()) /
                               scroll_step_);
  scroll_bar_.setRange(0, count);
}

void HorizontalScrollBarController::OnScroll(int pos) {
  if (updating_) {
    return;
  }

  base::AutoReset updating{&updating_, true};

  auto view_range = axis_.range();
  double min = pos * (scroll_range_.delta() - view_range.delta()) /
               scroll_bar_.maximum();
  axis_.SetRange({min, min + view_range.delta()});
}

void HorizontalScrollBarController::OnViewRangeChanged(
    const GraphRange& view_range) {
  if (updating_) {
    return;
  }

  base::AutoReset updating{&updating_, true};

  double scroll_delta = scroll_range_.delta() - view_range.delta();
  if (abs(scroll_delta) < std::numeric_limits<double>::epsilon()) {
    return;
  }

  int pos =
      static_cast<int>(scroll_bar_.maximum() *
                       (view_range.low() - scroll_range_.low()) / scroll_delta);

  // Tolerates `pos` being out of range, as it will be clamped by Qt.
  scroll_bar_.setValue(pos);
}

}  // namespace views
