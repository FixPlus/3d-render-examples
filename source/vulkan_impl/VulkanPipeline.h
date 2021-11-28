//
// Created by Бушев Дмитрий on 06.11.2021.
//

#ifndef RENDERAPITEST_VULKANPIPELINE_H
#define RENDERAPITEST_VULKANPIPELINE_H

#include "RenderInterface.h"
#include "vulkan_impl/util/UniversalContainer.h"
#include <vulkan/vulkan.h>

#include <utility>

namespace APITest {

class VulkanRenderImpl;

class VulkanPipelineManager;

class VulkanRenderPass;

class VulkanPipeline : virtual public Pipeline {
protected:
  VulkanPipelineManager *parent_;
  /**
   *
   * Single vulkan pipeline derives from render pass instance, though their
   * layout is identical and for end user their semantics are the same. That's
   * why they are aggregated in this class.
   *
   * */
  std::map<VkRenderPass, VkPipeline> perPassMap_{};
  VkPipelineLayout layoutHandle = VK_NULL_HANDLE;

public:
  VulkanPipeline(VulkanPipelineManager *parent) : parent_(parent) {}

  VkPipelineLayout getLayout() const { return layoutHandle; };

  virtual VkPipeline get(VulkanRenderPass *pass) = 0;

  ~VulkanPipeline() override;
};

class VulkanGraphicsPipeline final : public VulkanPipeline,
                                     public GraphicsPipeline {

  GraphicsPipelineLayout layout_;

public:
  VulkanGraphicsPipeline(VulkanPipelineManager *parent,
                         GraphicsPipelineLayout layout);

  GraphicsPipelineLayout const &layout() const override { return layout_; };

  VkPipeline get(VulkanRenderPass *pass) override;

  ~VulkanGraphicsPipeline() override;
};

class VulkanPipelineManager final {
  VkPipelineCache pipelineCache = VK_NULL_HANDLE;
  VulkanRenderImpl *parent_ = nullptr;

public:
  VulkanPipelineManager(VulkanRenderImpl *parent);

  GraphicsPipelineRef get(GraphicsPipelineLayout const &desc);

  ~VulkanPipelineManager();

  friend class VulkanPipeline;
  friend class VulkanGraphicsPipeline;
};

} // namespace APITest
#endif // RENDERAPITEST_VULKANPIPELINE_H
