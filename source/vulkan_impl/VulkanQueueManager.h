//
// Created by Бушев Дмитрий on 28.10.2021.
//

#ifndef RENDERAPITEST_VULKANQUEUEMANAGER_H
#define RENDERAPITEST_VULKANQUEUEMANAGER_H

#include "vulkan/vulkan.h"

namespace APITest {

class VulkanDevice;

class VulkanQueueManager final {
  VulkanDevice *device_;
  VkQueue queue_;
  VkQueue transferQueue_;
  VkCommandPool commandPool_;

public:
  void flush(VkCommandBuffer cmdBuffer, VkQueue queue, bool free = true) const;
  explicit VulkanQueueManager(VulkanDevice *device);
  VkQueue getMainQueue() const { return queue_; };
  VkQueue getTransferQueue() const { return transferQueue_; }

  VkResult createPrimaryCommandBuffers(VkCommandBuffer *buffers, size_t count,
                                       bool begin = false) const;

  void copyBuffer(VkBuffer src, VkBuffer dst, VkBufferCopy region) const;
  ~VulkanQueueManager();
};
} // namespace APITest
#endif // RENDERAPITEST_VULKANQUEUEMANAGER_H
