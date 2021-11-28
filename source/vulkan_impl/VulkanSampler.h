//
// Created by Бушев Дмитрий on 15.11.2021.
//

#ifndef RENDERAPITEST_VULKANSAMPLER_H
#define RENDERAPITEST_VULKANSAMPLER_H

#include "RenderInterface.h"
#include <vulkan/vulkan.h>

namespace APITest {

class VulkanRenderImpl;

class VulkanSampler : public Sampler {
  VulkanRenderImpl *parent_;
  VkSampler sampler_ = VK_NULL_HANDLE;

public:
  VulkanSampler(VulkanRenderImpl *parent, SamplerDesc const &desc);

  VkSampler get() const { return sampler_; }

  ~VulkanSampler() override;
};

} // namespace APITest
#endif // RENDERAPITEST_VULKANSAMPLER_H
