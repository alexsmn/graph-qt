#pragma once

#include "graph_qt/model/graph_data_source.h"
#include "graph_qt/model/graph_types.h"

#include <cstdlib>
#include <span>

namespace views {

class TestPointEnumerator : public PointEnumerator {
 public:
  explicit TestPointEnumerator(std::span<const GraphPoint> points)
      : points_{points} {}

  virtual size_t GetCount() const override { return points_.size(); }

  virtual bool EnumNext(GraphPoint& value) override {
    if (points_.empty()) {
      return false;
    }

    value = points_[0];
    points_ = points_.subspan(1);
    return true;
  }

 private:
  std::span<const GraphPoint> points_;
};

class TestDataSource : public GraphDataSource {
 public:
  TestDataSource() {
    for (int i = 0; i < kInitialCount; ++i) {
      points_.emplace_back(kXOffset + i, i);
    }
  }

  void AddPoint() {
    points_.emplace_back(kXOffset + points_.size(), points_.size());

    if (observer_) {
      observer_->OnDataSourceCurrentValueChanged();
      observer_->OnDataSourceHistoryChanged();
    }
  }

  virtual double GetCurrentValue() const override { return points_.back().y; }

  virtual PointEnumerator* EnumPoints(double from,
                                      double to,
                                      bool include_left_bound,
                                      bool include_right_bound) override {
    return new TestPointEnumerator{points_};
  }

  virtual GraphRange GetHorizontalRange() const override {
    return {points_.front().x, points_.back().x};
  }

  static const int kInitialCount = 100;
  static const int kXOffset = 1000;

 protected:
  std::vector<GraphPoint> points_;
};

struct VirtualDataset {
  double value_at(size_t index) const {
    return std::labs(index % (ramp_count_ * 2) - ramp_count_) * step_;
  }

  GraphPoint at(size_t index) const {
    return {horizontal_min_ + index * step_, value_at(index)};
  }

  std::optional<std::pair<size_t, size_t>> index_range(double from,
                                                       double to) const {
    if (to < horizontal_min_) {
      return std::nullopt;
    }

    size_t from_index = (from - horizontal_min_) / step_;
    if (from_index >= count_) {
      return std::nullopt;
    }

    size_t to_index = (to - horizontal_min_) / step_;
    to_index = std::min(to_index, count_ - 1);

    return std::pair{from_index, to_index};
  }

  double current() const { return value_at(count_ - 1); }

  GraphRange horizontal_range() const {
    return {horizontal_min_, horizontal_min_ + count_ * step_};
  }

  double horizontal_min_;
  double step_;
  size_t count_;
  size_t ramp_count_;
};

class VirtualPointEnumerator : public PointEnumerator {
 public:
  // Indexes are specified including bounds.
  VirtualPointEnumerator(const VirtualDataset& dataset,
                         size_t first_index,
                         size_t last_index)
      : dataset_{dataset}, last_index_{last_index}, index_{first_index} {}

  virtual size_t GetCount() const override { return last_index_ - index_ + 1; }

  virtual bool EnumNext(GraphPoint& value) override {
    if (index_ > last_index_) {
      return false;
    }

    value = dataset_.at(index_);
    ++index_;
    return true;
  }

 private:
  const VirtualDataset& dataset_;
  const size_t last_index_;
  size_t index_;
};

class VirtualDataSource : public GraphDataSource {
 public:
  explicit VirtualDataSource(VirtualDataset& dataset) : dataset_{dataset} {}

  void AddPoint() {
    ++dataset_.count_;

    if (observer_) {
      observer_->OnDataSourceCurrentValueChanged();
      observer_->OnDataSourceHistoryChanged();
    }
  }

  virtual double GetCurrentValue() const override { return dataset_.current(); }

  virtual PointEnumerator* EnumPoints(double from,
                                      double to,
                                      bool include_left_bound,
                                      bool include_right_bound) override {
    auto indexes = dataset_.index_range(from, to);
    if (!indexes) {
      return nullptr;
    }
    return new VirtualPointEnumerator{dataset_, indexes->first,
                                      indexes->second};
  }

  virtual GraphRange GetHorizontalRange() const override {
    return dataset_.horizontal_range();
  }

 protected:
  VirtualDataset& dataset_;
};

}  // namespace views
