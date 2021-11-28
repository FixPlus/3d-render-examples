//
// Created by Бушев Дмитрий on 31.10.2021.
//

#include "VulkanShaderManager.h"
#include "vulkan_impl/VulkanDevice.h"
#include <fstream>
#include <vulkan/vulkan.h>
#include <vulkan_impl/util/macro.h>

uint32_t APITest::VulkanShaderManager::loadShader(const std::string &filename,
                                                  VkShaderStageFlagBits stage,
                                                  std::string const &name) {

  std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);

  if (is.is_open()) {
    size_t size = is.tellg();
    is.seekg(0, std::ios::beg);
    char *shaderCode = new char[size];
    is.read(shaderCode, size);
    is.close();

    if (size == 0)
      throw std::runtime_error(
          "[VK][ERROR]: shader module load failed: file is empty. Filename: " +
          filename);

    VkShaderModule shaderModule;
    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = size;
    moduleCreateInfo.pCode = (uint32_t *)shaderCode;

    VK_CHECK_RESULT(vkCreateShaderModule(parent_->getVulkanDevice()->get(),
                                         &moduleCreateInfo, NULL,
                                         &shaderModule))

    delete[] shaderCode;

    auto ret = shaders_.push({shaderModule, name, stage});

    return ret;
  } else {
    throw std::runtime_error("[VK][ERROR]: shader module load failed: could "
                             "not open file. Filename: " +
                             filename);
  }
}

VkShaderModule APITest::VulkanShaderManager::getModule(uint32_t key) const {
  return shaders_.get(key).module;
}

void APITest::VulkanShaderManager::clear() {
  auto device = parent_->getVulkanDevice()->get();
  for (auto &shader : shaders_) {
    if (shader.second.module != VK_NULL_HANDLE) {
      vkDestroyShaderModule(device, shader.second.module, nullptr);
    }
  }
  shaders_.clear();
}

APITest::VulkanShaderManager::~VulkanShaderManager() { clear(); }
