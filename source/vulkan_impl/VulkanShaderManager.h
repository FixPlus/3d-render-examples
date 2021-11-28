//
// Created by Бушев Дмитрий on 31.10.2021.
//

#ifndef RENDERAPITEST_VULKANSHADERMANAGER_H
#define RENDERAPITEST_VULKANSHADERMANAGER_H

#include "VulkanRenderImpl.h"
#include "vulkan_impl/util/UniversalContainer.h"
#include <map>
#include <string>
#include <vulkan/vulkan.h>

namespace APITest {

struct ShaderModule {

  VkShaderModule module = VK_NULL_HANDLE;
  std::string name = "unnamed";
  VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT;

  ShaderModule(VkShaderModule pT, const std::string basicString,
               VkShaderStageFlagBits bits)
      : module(pT), name(basicString), stage(bits){};
};

class VulkanShaderManager {
  UniversalContainer<ShaderModule> shaders_;
  VulkanRenderImpl *parent_ = nullptr;

public:
  explicit VulkanShaderManager(VulkanRenderImpl *parent) : parent_(parent) {}

  uint32_t loadShader(std::string const &filename, VkShaderStageFlagBits stage,
                      std::string const &name = "unnamed");

  VkShaderModule getModule(uint32_t key) const;

  void clear();

  ~VulkanShaderManager();
};
} // namespace APITest
#endif // RENDERAPITEST_VULKANSHADERMANAGER_H
