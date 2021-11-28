//
// Created by Бушев Дмитрий on 07.11.2021.
//

#ifndef RENDERAPITEST_RENDERJOB_H
#define RENDERAPITEST_RENDERJOB_H

#include "vulkan_impl/VulkanRenderPass.h"
#include <RenderInterface.h>
#include <map>
#include <vector>
#include <vulkan/vulkan.h>

namespace APITest {

class VulkanOnscreenRenderPass;

class VulkanCommand;

class VulkanRenderImpl;

class RenderJob {
  VulkanRenderImpl *parent_ = nullptr;
  struct RenderUnit {
    RenderPassRef pass;
    std::vector<VkFramebuffer> frameBuffers{};
    explicit RenderUnit(RenderPassRef renderPass)
        : pass(std::move(renderPass)) {}
  };
  std::vector<RenderUnit> renderSequence;
  void compileRenderPass(RenderPassRef desc);

public:
  RenderJob(VulkanRenderImpl *parent, RenderPassRef desc);

  void resetFrameBuffers();

  void compile(int swapChainImage, VkCommandBuffer commandBuffer);

  ~RenderJob();
};
} // namespace APITest
#endif // RENDERAPITEST_RENDERJOB_H
