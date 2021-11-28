//
// Created by Бушев Дмитрий on 14.11.2021.
//

#ifndef RENDERAPITEST_VULKANDESCRIPTORMANAGER_H
#define RENDERAPITEST_VULKANDESCRIPTORMANAGER_H

#include "RenderInterface.h"
#include <algorithm>
#include <vulkan/vulkan.h>

namespace APITest {

class VulkanRenderImpl;
class VulkanDescriptorPool;
class VulkanDescriptorManager;

class VulkanDescriptorSetLayout final : public DescriptorSetLayout {
  VulkanDescriptorManager *parent_;
  VkDescriptorSetLayout setLayout_ = VK_NULL_HANDLE;

public:
  VkDescriptorSetLayout layout() const { return setLayout_; }

  VulkanDescriptorSetLayout(VulkanDescriptorManager *parent,
                            std::vector<DescriptorLayout> const &desc);

  UniformDescriptorSetRef allocateNewSet(UniformDescriptor *descriptors,
                                         int count) override;

  ~VulkanDescriptorSetLayout() override;
};

class VulkanDescriptorSet : public UniformDescriptorSet {
  VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;

public:
  VulkanDescriptorSet(VulkanDescriptorPool *parent,
                      VkDescriptorSetLayout layout,
                      UniformDescriptor *descriptor, int count);

  VkDescriptorSet get() const { return descriptorSet_; };
  ~VulkanDescriptorSet() override =
      default; // VkDescriptorSet is freed when corresponding pool is freed.
};

class VulkanDescriptorPool {
  VkDescriptorPool pool = VK_NULL_HANDLE;
  VulkanRenderImpl *parent_;
  std::vector<UniformDescriptorSetRef> allocatedSets;
  std::vector<std::pair<VkDescriptorType, std::pair<size_t, size_t>>>
      descriptors;
  size_t maxSets_;

public:
  VulkanDescriptorPool(VulkanRenderImpl *parent,
                       std::vector<VkDescriptorPoolSize> const &sizes,
                       int maxSets);

  ~VulkanDescriptorPool();

  UniformDescriptorSetRef allocateDescriptorSet(VkDescriptorSetLayout layout,
                                                UniformDescriptor *descriptor,
                                                int count);

  size_t setsAvailable() const { return maxSets_ - allocatedSets.size(); };

  bool canBeFreed() {
    return std::all_of(allocatedSets.begin(), allocatedSets.end(),
                       [](UniformDescriptorSetRef const &set) {
                         return set.use_count() == 1;
                       });
  }

  friend class VulkanDescriptorSet;
};
class VulkanDescriptorManager {
  VulkanRenderImpl *parent_;
  std::vector<std::unique_ptr<VulkanDescriptorPool>> pools_;

public:
  explicit VulkanDescriptorManager(VulkanRenderImpl *parent)
      : parent_(parent){};
  UniformDescriptorSetRef allocateDescriptorSet(VkDescriptorSetLayout layout,
                                                UniformDescriptor *descriptor,
                                                int count);
  DescriptorSetLayoutRef
  createNewDescriptorSetLayout(std::vector<DescriptorLayout> const &desc) {
    return DescriptorSetLayoutRef{new VulkanDescriptorSetLayout(this, desc)};
  };

  friend class VulkanDescriptorSetLayout;
};
} // namespace APITest
#endif // RENDERAPITEST_VULKANDESCRIPTORMANAGER_H
