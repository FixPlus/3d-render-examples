//
// Created by Бушев Дмитрий on 09.11.2021.
//

#ifndef RENDERAPITEST_VULKANCOMMANDRECORDER_H
#define RENDERAPITEST_VULKANCOMMANDRECORDER_H

#include "RenderInterface.h"
#include <vulkan/vulkan.h>

namespace APITest {

class VulkanRenderPass;
class VulkanPipeline;
struct VulkanCommandRecorder final : public CommandRecorder {
  VkCommandBuffer bufferToRecord = VK_NULL_HANDLE;
  VulkanRenderPass *currentPass = nullptr;
  VulkanPipeline *currentPipeline = nullptr;
  VkPipelineBindPoint pipelineBindPoint;

  void bindPipeline(Pipeline *pipeline) override;
  void bindVertexBuffer(VertexBuffer *, uint32_t binding) override;
  void bindIndexBuffer(IndexBuffer *buffer) override;
  void draw(uint32_t vertexCount, uint32_t firstIndex) override;
  void bindDescriptorSet(UniformDescriptorSet *set) override;
  void drawIndexed(uint32_t indexCount, uint32_t firstIndex,
                   uint32_t vertexOffset) override;
};
} // namespace APITest
#endif // RENDERAPITEST_VULKANCOMMANDRECORDER_H
