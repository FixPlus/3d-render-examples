//
// Created by Бушев Дмитрий on 01.11.2021.
//

#ifndef RENDERAPITEST_VULKANRENDERPASS_H
#define RENDERAPITEST_VULKANRENDERPASS_H

#include "RenderInterface.h"
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace APITest {

class VulkanRenderImpl;

class VulkanRenderPass : public virtual RenderPass {
protected:
  VkRenderPass pass_ = VK_NULL_HANDLE;
  VulkanRenderImpl *parent_;

public:
  explicit VulkanRenderPass(VulkanRenderImpl *device) : parent_(device) {}

  VkRenderPass get() const { return pass_; }

  virtual void compile() = 0;

  VulkanRenderPass(VulkanRenderPass const &another) = delete;
  VulkanRenderPass(VulkanRenderPass &&another) noexcept {
    pass_ = another.pass_;
    parent_ = another.parent_;
    another.pass_ = VK_NULL_HANDLE;
  }
  VulkanRenderPass const &operator=(VulkanRenderPass const &another) = delete;
  VulkanRenderPass &operator=(VulkanRenderPass &&another) noexcept {
    pass_ = another.pass_;
    parent_ = another.parent_;
    another.pass_ = VK_NULL_HANDLE;
    return *this;
  }

  virtual void begin(VkCommandBuffer cmdBuffer, VkFramebuffer framebuffer) = 0;
  virtual void end(VkCommandBuffer cmdBuffer) = 0;
  ~VulkanRenderPass() override;
};

class VulkanColorPass : virtual public VulkanRenderPass,
                        virtual public ColorPass {
protected:
  Image::Extents extents;

public:
  virtual bool hasDepthBuffer() const = 0;

  Image::Extents getFramebufferExtents() const override { return extents; };
};
class VulkanDepthBuffer;
class VulkanMemoryManager;

class VulkanOnscreenRenderPass final : public VulkanColorPass,
                                       public OnscreenRenderPass {
  std::vector<VkClearValue> clearValues;

  // .first = color, .second = depth
  std::pair<uint32_t, uint32_t> bindings;

  struct AttachInfo {
    uint32_t binding;
  };

  VulkanDepthBuffer *depthBuffer = nullptr;

  AttachInfo colorAttachInfo{0};
  AttachInfo depthBufferAttachInfo{1};

public:
  /** interface */

  void setColorBufferIndex(uint32_t binding) override;
  void enableDepthBuffer(uint32_t binding) override;
  bool hasDepthBuffer() const override { return depthBuffer != nullptr; };

  void compile() override;

  VkFramebuffer allocateFrameBuffer(VkImageView swapChainImage);

  void changeExtents(int newWidth, int newHeight);

  explicit VulkanOnscreenRenderPass(VulkanRenderImpl *parent);
  void begin(VkCommandBuffer cmdBuffer, VkFramebuffer framebuffer) override;
  void end(VkCommandBuffer cmdBuffer) override;

  ~VulkanOnscreenRenderPass() override;
};

} // namespace APITest
#endif // RENDERAPITEST_VULKANRENDERPASS_H
