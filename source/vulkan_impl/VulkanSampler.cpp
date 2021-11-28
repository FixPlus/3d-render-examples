//
// Created by Бушев Дмитрий on 15.11.2021.
//

#include "VulkanSampler.h"
#include "util/VulkanInitializers.h"
#include "util/macro.h"
#include "vulkan_impl/VulkanDevice.h"
#include "vulkan_impl/VulkanRenderImpl.h"

APITest::VulkanSampler::VulkanSampler(APITest::VulkanRenderImpl *parent,
                                      const APITest::SamplerDesc &desc)
    : parent_(parent) {
  VkSamplerCreateInfo samplerCreateInfo = initializers::samplerCreateInfo();
  samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.mipLodBias = 0.0f;
  samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
  samplerCreateInfo.minLod = 0.0f;
  // Max level-of-detail should match mip level count
  samplerCreateInfo.maxLod = 0.0f;
  // Only enable anisotropic filtering if enabled on the device
  samplerCreateInfo.maxAnisotropy =
      parent_->getVulkanDevice()->enabledFeatures.samplerAnisotropy
          ? parent_->getVulkanDevice()->properties.limits.maxSamplerAnisotropy
          : 1.0f;
  samplerCreateInfo.anisotropyEnable =
      parent_->getVulkanDevice()->enabledFeatures.samplerAnisotropy;
  samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  VK_CHECK_RESULT(vkCreateSampler(parent_->getVulkanDevice()->get(),
                                  &samplerCreateInfo, nullptr, &sampler_))
}

APITest::VulkanSampler::~VulkanSampler() {
  vkDestroySampler(parent_->getVulkanDevice()->get(), sampler_, nullptr);
}
