#include "graph_qt/horizontal_scroll_bar_controller.h"

#include "graph_qt/graph_axis.h"

#include <QScrollBar>
#include <QScopedValueRollback>

namespace views {

namespace {

// Advance on 10% of the view range on each scroll.
const int kScrollPageStep = 10;

bool IsZero(double value) {
  return abs(value) < std::numeric_limits<double>::epsilon();
}

}  // namespace

HorizontalScrollBarController::HorizontalScrollBarController(
    QScrollBar& scroll_bar,
    GraphAxis& horizontal_axis)
    : scroll_bar_{scroll_bar}, axis_{horizontal_axis} {
  scroll_bar_.setVisible(visible_);
  scroll_bar_.setPageStep(kScrollPageStep);

  QObject::connect(
      &axis_, &GraphAxis::rangeChanged,
      std::bind_front(&HorizontalScrollBarController::OnViewRangeChanged,
                      this));

  QObject::connect(
      &scroll_bar_, &QScrollBar::valueChanged,
      std::bind_front(&HorizontalScrollBarController::OnScroll, this));
}

void HorizontalScrollBarController::SetVisible(bool visible) {
  visible_ = visible;
  scroll_bar_.setVisible(visible);
}

void HorizontalScrollBarController::SetScrollRange(const GraphRange& range) {
  if (scroll_range_ == range) {
    return;
  }

  scroll_range_ = range;

  QScopedValueRollback<bool> updating(updating_, true);

  auto view_range = axis_.range();
  if (view_range.empty() || scroll_range_.empty()) {
    scroll_bar_.setRange(0, 0);
    return;
  }

  scroll_step_ = view_range.delta() / kScrollPageStep;

  scroll_bar_.setRange(0, CalculateScrollRange());
  scroll_bar_.setValue(CalculateScrollPos());
}

void HorizontalScrollBarController::OnScroll(int pos) {
  if (updating_) {
    return;
  }

  auto view_range = CalculateViewRange(pos);
  if (view_range.empty()) {
    return;
  }

  QScopedValueRollback<bool> updating(updating_, true);
  axis_.SetRange(view_range);
}

GraphRange HorizontalScrollBarController::CalculateViewRange(int pos) const {
  if (scroll_bar_.maximum() == 0) {
    return {};
  }

  auto view_range = axis_.range();
  double min =
      scroll_range_.low() + pos * (scroll_range_.delta() - view_range.delta()) /
                                scroll_bar_.maximum();
  return {min, min + view_range.delta(), axis_.range().kind()};
}

void HorizontalScrollBarController::OnViewRangeChanged() {
  if (updating_) {
    return;
  }

  QScopedValueRollback<bool> updating(updating_, true);

  int pos = CalculateScrollPos();
  scroll_bar_.setValue(pos);
}

int HorizontalScrollBarController::CalculateScrollRange() const {
  const auto& view_range = axis_.range();

  if (IsZero(scroll_step_)) {
    return 0;
  }

  return static_cast<int>((scroll_range_.delta() - view_range.delta()) /
                          scroll_step_);
}

int HorizontalScrollBarController::CalculateScrollPos() const {
  const auto& view_range = axis_.range();

  double scroll_delta = scroll_range_.delta() - view_range.delta();
  if (IsZero(scroll_delta)) {
    return 0;
  }

  return static_cast<int>(scroll_bar_.maximum() *
                          (view_range.low() - scroll_range_.low()) /
                          scroll_delta);
}

}  // namespace views
