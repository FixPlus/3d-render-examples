//
// Created by Бушев Дмитрий on 28.10.2021.
//

#include "VulkanQueueManager.h"
#include "vulkan_impl/VulkanDevice.h"
#include "vulkan_impl/util/VulkanInitializers.h"
#include <stdexcept>
#include <vulkan_impl/util/macro.h>

APITest::VulkanQueueManager::VulkanQueueManager(APITest::VulkanDevice *device)
    : device_(device) {

  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = device_->queueFamilyIndices.graphics;
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (auto err = (vkCreateCommandPool(device_->get(), &cmdPoolInfo, nullptr,
                                      &commandPool_) -
                  VK_SUCCESS))
    throw std::runtime_error("[ERROR][VkAPI]: Could not create a command pool "
                             "on given device. Err code: " +
                             std::to_string(err + VK_SUCCESS));

  // Get a graphics queue from the device
  vkGetDeviceQueue(device->get(), device->queueFamilyIndices.graphics, 0,
                   &queue_);

  // Get a transfer queue from the device
  vkGetDeviceQueue(device->get(), device->queueFamilyIndices.transfer, 0,
                   &transferQueue_);
}

APITest::VulkanQueueManager::~VulkanQueueManager() {
  vkDestroyCommandPool(device_->get(), commandPool_, nullptr);
}

VkResult APITest::VulkanQueueManager::createPrimaryCommandBuffers(
    VkCommandBuffer *buffers, size_t count, bool begin) const {
  VkCommandBufferAllocateInfo cmdBufAllocateInfo =
      initializers::commandBufferAllocateInfo(commandPool_,
                                              VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                              static_cast<uint32_t>(count));
  if (auto result = vkAllocateCommandBuffers(device_->get(),
                                             &cmdBufAllocateInfo, buffers))
    return result;

  if (begin) {
    VkCommandBufferBeginInfo cmdBufInfo =
        initializers::commandBufferBeginInfo();
    for (int i = 0; i < count; i++)
      VK_CHECK_RESULT(vkBeginCommandBuffer(buffers[i], &cmdBufInfo))
  }

  return VK_SUCCESS;
}

void APITest::VulkanQueueManager::copyBuffer(VkBuffer src, VkBuffer dst,
                                             VkBufferCopy region) const {
  VkCommandBuffer copyCommand;
  VK_CHECK_RESULT(createPrimaryCommandBuffers(&copyCommand, 1, true));

  vkCmdCopyBuffer(copyCommand, src, dst, 1, &region);

  flush(copyCommand, transferQueue_);
}

void APITest::VulkanQueueManager::flush(VkCommandBuffer cmdBuffer,
                                        VkQueue queue, bool free) const {
  VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

  VkSubmitInfo submitInfo = initializers::submitInfo();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffer;
  // Create fence to ensure that the command buffer has finished executing
  VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
  VkFence fence;
  VK_CHECK_RESULT(vkCreateFence(device_->get(), &fenceInfo, nullptr, &fence));
  // Submit to the queue
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
  // Wait for the fence to signal that command buffer has finished executing
  VK_CHECK_RESULT(vkWaitForFences(device_->get(), 1, &fence, VK_TRUE,
                                  DEFAULT_FENCE_TIMEOUT));
  vkDestroyFence(device_->get(), fence, nullptr);
  if (free) {
    vkFreeCommandBuffers(device_->get(), commandPool_, 1, &cmdBuffer);
  }
}
