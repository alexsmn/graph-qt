#pragma once

#include "graph_qt/model/graph_data_source.h"
#include "graph_qt/model/graph_types.h"

#include <QTimer>
#include <span>

class TestPointEnumerator : public views::PointEnumerator {
 public:
  explicit TestPointEnumerator(std::span<const views::GraphPoint> points)
      : points_{points} {}

  virtual size_t GetCount() const override { return points_.size(); }

  virtual bool EnumNext(views::GraphPoint& value) override {
    if (points_.empty()) {
      return false;
    }

    value = points_[0];
    points_ = points_.subspan(1);
    return true;
  }

 private:
  std::span<const views::GraphPoint> points_;
};

class TestDataSource : public views::GraphDataSource {
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

  virtual views::PointEnumerator* EnumPoints(
      double from,
      double to,
      bool include_left_bound,
      bool include_right_bound) override {
    return new TestPointEnumerator{points_};
  }

  virtual views::GraphRange GetHorizontalRange() const override {
    return {points_.front().x, points_.back().x};
  }

  static const int kInitialCount = 100;
  static const int kXOffset = 1000;

 protected:
  std::vector<views::GraphPoint> points_;
};

class UpdatingTestDataSource : public TestDataSource {
 public:
  explicit UpdatingTestDataSource(std::chrono::milliseconds update_period) {
    timer_.setInterval(update_period);

    QObject::connect(&timer_, &QTimer::timeout, [this] { AddPoint(); });

    timer_.start();
  }

 private:
  QTimer timer_;
};
