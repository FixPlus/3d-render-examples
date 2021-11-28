//
// Created by Бушев Дмитрий on 17.11.2021.
//

#include "common.h"

void APITest::StatisticsRecorder::endFrame() {
  if (frameBlockStack.empty())
    return;
  while (frameBlockStack.size() != 1)
    pop();

  stamp("End");
  frameBlock.childs.resize(frameBlockStack.top().second);

  totalFrames++;

  double overallFrameTime =
      std::chrono::duration<double, std::milli>(
          frameBlock.childs.back()->stamp - frameBlock.childs.front()->stamp)
          .count();

  statistics.lastFrameTime = overallFrameTime / 1000.0;
  statistics.overallTime += statistics.lastFrameTime;

  double avgFps = 1000.0 / frameBlock.avgTime.record();
  const uint32_t recordRate = avgFps + 10;
  frameBlock.avgTime.accumulate(overallFrameTime, recordRate);
  auto topLevelRecordsSize = frameBlock.childs.size();
  std::stack<std::pair<std::pair<FrameBlock *, FrameBlock *>, int>>
      traversalStack;

  for (int i = 0; i < topLevelRecordsSize - 1; ++i) {
    auto *child = frameBlock.childs.at(i).get();

    traversalStack.push({{child, frameBlock.childs.at(i + 1).get()}, 0});

    while (!traversalStack.empty()) {
      auto [blocks, currentChild] = traversalStack.top();
      auto *currentBlock = blocks.first;
      auto *nextBlock = blocks.second;

      if (currentChild < currentBlock->childs.size()) {
        traversalStack.top().second++;
        traversalStack.push(
            {{currentBlock->childs.at(currentChild).get(),
              currentChild == currentBlock->childs.size() - 1
                  ? nextBlock
                  : currentBlock->childs.at(currentChild + 1).get()},
             0});
        continue;
      }

      double blockTime = std::chrono::duration<double, std::milli>(
                             nextBlock->stamp - currentBlock->stamp)
                             .count();
      currentBlock->avgTime.accumulate(blockTime, recordRate);
      currentBlock->lastTime = blockTime;
      traversalStack.pop();
    }
  }

  if (totalFrames % 100 == 5) {
    std::stack<
        std::pair<std::pair<FrameBlock *, FrameStatistics::TimeBlock *>, int>>
        statStack;
    statistics.frameTimeBlock.duration.first = frameBlock.avgTime.record();
    statistics.frameTimeBlock.duration.second =
        1000.0 / statistics.frameTimeBlock.duration.first;

    statStack.push({{&frameBlock, &statistics.frameTimeBlock}, 0});

    while (!statStack.empty()) {
      auto [blocks, currentChild] = statStack.top();
      FrameStatistics::TimeBlock *currentStatBlock = blocks.second;
      FrameBlock *currentFrameBlock = blocks.first;

      if (currentFrameBlock->childs.size() !=
          currentStatBlock->children.capacity())
        currentStatBlock->children.reserve(currentFrameBlock->childs.size());

      if (currentChild < currentFrameBlock->childs.size()) {
        statStack.top().second++;
        if (currentStatBlock->children.size() <= currentChild) {
          currentStatBlock->children.emplace_back(
              std::make_unique<FrameStatistics::TimeBlock>());
        }
        statStack.push({{currentFrameBlock->childs.at(currentChild).get(),
                         currentStatBlock->children.at(currentChild).get()},
                        0});
        continue;
      }

      currentStatBlock->name = currentFrameBlock->name;
      currentStatBlock->duration = {currentFrameBlock->avgTime.record(),
                                    currentFrameBlock->avgTime.record() /
                                        frameBlock.avgTime.record() * 100.0};
      statStack.pop();
    }
    statistics.frameTimeBlock.children.resize(
        statistics.frameTimeBlock.children.size() - 1);
    statistics.frameTimeBlock.duration.second = avgFps;

    statistics.integerMetrics.resize(integerMetrics_.size());
    int i = 0;
    for (auto &metric : integerMetrics_) {
      if (!metric.second.first)
        continue;
      statistics.integerMetrics.at(i) = {metric.first, metric.second.second};
      i++;
    }
    i = 0;
    for (auto &metric : floatMetrics_) {
      if (!metric.second.first)
        continue;
      statistics.doubleMetrics.at(i) = {metric.first, metric.second.second};
      i++;
    }
  }
}
