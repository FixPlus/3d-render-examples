//
// Created by Бушев Дмитрий on 09.11.2021.
//

#include "VulkanCommandRecorder.h"
#include "vulkan_impl/VulkanDescriptorManager.h"
#include "vulkan_impl/VulkanMemoryManager.h"
#include "vulkan_impl/VulkanPipeline.h"
#include <cassert>
#include <stdexcept>

void APITest::VulkanCommandRecorder::bindPipeline(APITest::Pipeline *pipeline) {
  currentPipeline = dynamic_cast<VulkanPipeline *>(pipeline);
  if (auto *vulkanPipeline =
          dynamic_cast<VulkanGraphicsPipeline *>(currentPipeline)) {
    vkCmdBindPipeline(bufferToRecord,
                      VK_PIPELINE_BIND_POINT_GRAPHICS /* TODO */,
                      vulkanPipeline->get(currentPass));
    pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  } else {
    throw std::runtime_error(
        "[Vulkan][ERROR] bindPipeline() - invalid pipeline object.");
  }
}

void APITest::VulkanCommandRecorder::draw(uint32_t vertexCount,
                                          uint32_t firstIndex) {
  vkCmdDraw(bufferToRecord, vertexCount, 1, firstIndex, 0);
}

static VkIndexType convertIndexType(APITest::IndexBuffer::Type type) {
  switch (type) {
  case APITest::IndexBuffer::Type::INDEX_TYPE_UINT_32:
    return VK_INDEX_TYPE_UINT32;
  case APITest::IndexBuffer::Type::INDEX_TYPE_UINT_16:
    return VK_INDEX_TYPE_UINT16;
  case APITest::IndexBuffer::Type::INDEX_TYPE_UINT_8:
    return VK_INDEX_TYPE_UINT8_EXT;
  }

  throw std::runtime_error("[VULKAN][ERROR]: invalid index type");
}

void APITest::VulkanCommandRecorder::bindVertexBuffer(
    APITest::VertexBuffer *buffer, uint32_t binding) {
  auto *vkBuffer = dynamic_cast<VulkanVertexBuffer *>(buffer);
  if (!vkBuffer)
    throw std::runtime_error(
        "[VULKAN][ERROR]: invalid vertex buffer object used.");
  VkDeviceSize offset = 0;
  auto buf = vkBuffer->buffer();
  vkCmdBindVertexBuffers(bufferToRecord, binding, 1, &buf, &offset);
}

void APITest::VulkanCommandRecorder::bindDescriptorSet(
    APITest::UniformDescriptorSet *set) {
  if (auto *castSet = dynamic_cast<VulkanDescriptorSet *>(set)) {
    auto vkSet = castSet->get();
    vkCmdBindDescriptorSets(bufferToRecord, pipelineBindPoint,
                            currentPipeline->getLayout(), 0, 1, &vkSet, 0,
                            nullptr);
  } else {
    throw std::runtime_error(
        "[Vulkan][ERROR] bindDescriptorSet() - invalid descriptor set object.");
  }
}

void APITest::VulkanCommandRecorder::drawIndexed(uint32_t indexCount,
                                                 uint32_t firstIndex,
                                                 uint32_t vertexOffset) {
  vkCmdDrawIndexed(bufferToRecord, indexCount, 1, firstIndex, vertexOffset, 0);
}

void APITest::VulkanCommandRecorder::bindIndexBuffer(
    APITest::IndexBuffer *buffer) {
  auto *indexBuf = dynamic_cast<VulkanIndexBuffer *>(buffer);

  VkDeviceSize offset = 0;

  auto buf = indexBuf->buffer();

  vkCmdBindIndexBuffer(bufferToRecord, buf, offset,
                       convertIndexType(indexBuf->type));
}
