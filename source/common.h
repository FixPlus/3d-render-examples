//
// Created by Бушев Дмитрий on 17.11.2021.
//

#ifndef RENDERAPITEST_COMMON_H
#define RENDERAPITEST_COMMON_H

#include "RenderInterface.h"
#include <chrono>
#include <iomanip>
#include <map>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

namespace APITest {

template <typename T> class RecordAccumulator {
  T currentAccumRecord = static_cast<T>(0);
  int ticksSinceRecord = 0;
  T evaluatedRecord = static_cast<T>(0);

public:
  T record() const { return evaluatedRecord; }
  void accumulate(T value, uint32_t RecordRate) {
    currentAccumRecord += value;
    ticksSinceRecord++;
    if (ticksSinceRecord >= RecordRate) {
      evaluatedRecord = currentAccumRecord / static_cast<T>(ticksSinceRecord);
      ticksSinceRecord = 0;
      currentAccumRecord = static_cast<T>(0);
    }
  }
};

class StatisticsRecorder {
  using TimeStamp = std::chrono::time_point<std::chrono::high_resolution_clock,
                                            std::chrono::duration<double>>;
  template <typename T>
  using MetricsContainer = std::map<std::string, std::pair<bool, T>>;

  struct FrameBlock {
    std::string name;
    TimeStamp stamp;
    double lastTime;
    RecordAccumulator<double> avgTime;
    std::vector<std::unique_ptr<FrameBlock>> childs;
    FrameBlock(std::string nm = "") : name(std::move(nm)) {}
  } frameBlock;

  std::stack<std::pair<FrameBlock *, int>> frameBlockStack;

  std::vector<FrameBlock> frameBlocks_;

  MetricsContainer<int> integerMetrics_;
  MetricsContainer<float> floatMetrics_;

  template <typename T>
  static void addMetricInternal_(MetricsContainer<T> &container,
                                 std::string const &metricName, T metric) {
    auto it = container.find(metricName);
    if (it != container.end()) {
      it->second.first = true;
      it->second.second = metric;
    } else {
      container.insert_or_assign(metricName, std::pair<bool, T>{true, metric});
    }
  }
  template <typename T>
  static void unTouchMetric(MetricsContainer<T> &container) {
    for (auto &metric : container)
      metric.second.first = false;
  }

  void addTimeStampInternal_(std::string const &blockName) {
    auto [currentFrame, currentChild] = frameBlockStack.top();

    TimeStamp tick = std::chrono::high_resolution_clock::now();
    if (currentFrame->childs.size() <= currentChild) {
      currentFrame->childs.emplace_back(
          std::make_unique<FrameBlock>(blockName));
    } else {
      auto &savedInterval = currentFrame->childs.at(currentChild);
      if (savedInterval->name == blockName) {
        savedInterval->stamp = tick;
      } else {
        currentFrame->childs.emplace(currentFrame->childs.begin() +
                                         currentChild,
                                     std::make_unique<FrameBlock>(blockName));
      }
    }
    frameBlockStack.top().second++;
  }

  size_t totalFrames = 0;

  static constexpr const char *frameRecordID = "Frame start";

  FrameStatistics statistics;

public:
  void beginFrame(std::string const &blockName) {
    while (!frameBlockStack.empty())
      frameBlockStack.pop();

    frameBlockStack.push({&frameBlock, 0});
    unTouchMetric(integerMetrics_);
    unTouchMetric(floatMetrics_);

    addTimeStampInternal_(blockName);
  }

  void push(std::string const &parentBlockName) {
    addTimeStampInternal_(parentBlockName);
    auto [currentFrame, currentChild] = frameBlockStack.top();
    frameBlockStack.push({currentFrame->childs.at(currentChild - 1).get(), 0});
  }

  void stamp(std::string const &blockName) { addTimeStampInternal_(blockName); }

  void pop() {
    auto [currentFrame, currentChild] = frameBlockStack.top();
    currentFrame->childs.resize(currentChild);
    frameBlockStack.pop();
  }

  void endFrame();

  template <typename T>
  std::enable_if_t<std::is_integral<T>::value, void>
  addMetric(std::string const &metricName, T metric) {
    addMetricInternal_<T>(integerMetrics_, metricName, metric);
  }

  template <typename T>
  std::enable_if_t<std::is_floating_point<T>::value, void>
  addMetric(std::string const &metricName, T metric) {
    addMetricInternal_<T>(floatMetrics_, metricName, metric);
  }

  FrameStatistics const &stat() const { return statistics; }
};
} // namespace APITest
#endif // RENDERAPITEST_COMMON_H
