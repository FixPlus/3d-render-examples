//
// Created by Бушев Дмитрий on 14.11.2021.
//

#include "VulkanDescriptorManager.h"
#include "VulkanDevice.h"
#include "VulkanMemoryManager.h"
#include "VulkanRenderImpl.h"
#include "VulkanSampler.h"
#include "util/VulkanInitializers.h"
#include "util/macro.h"

#include <cassert>

namespace APITest {

static VkDescriptorType
convertInterfaceDescriptorType(APITest::UniformDescriptor const &desc) {
  if (std::holds_alternative<APITest::UniformBufferRef>(desc.descriptor))
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  if (std::holds_alternative<APITest::CombinedImageSampler>(desc.descriptor))
    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

  throw std::runtime_error("[VULKAN][ERROR]: invalid descriptor type.");
}

static VkDescriptorType
convertInterfaceDescriptorType(APITest::DescriptorLayout::Type type) {
  switch (type) {
  case APITest::DescriptorLayout::Type::UNIFORM_BUFFER:
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  case APITest::DescriptorLayout::Type::COMBINED_IMAGE_SAMPLER:
    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  }
  throw std::runtime_error("[VULKAN][ERROR]: invalid descriptor type.");
}

static VkShaderStageFlags convertStageFlags(APITest::ShaderStage stage) {
  switch (stage) {
  case ShaderStage::VERTEX:
    return VK_SHADER_STAGE_VERTEX_BIT;
  case ShaderStage::FRAGMENT:
    return VK_SHADER_STAGE_FRAGMENT_BIT;
  }
  throw std::runtime_error("[VULKAN][ERROR]: invalid shader stage.");
}

VulkanDescriptorSet::VulkanDescriptorSet(VulkanDescriptorPool *parent,
                                         VkDescriptorSetLayout layout,
                                         UniformDescriptor *descriptor,
                                         int count) {

  VkDescriptorSetAllocateInfo allocateInfo =
      initializers::descriptorSetAllocateInfo(parent->pool, &layout, 1);

  VK_CHECK_RESULT(
      vkAllocateDescriptorSets(parent->parent_->getVulkanDevice()->get(),
                               &allocateInfo, &descriptorSet_))

  std::vector<VkWriteDescriptorSet> writes;
  std::vector<std::unique_ptr<VkDescriptorBufferInfo>> bufferInfos;
  std::vector<std::unique_ptr<VkDescriptorImageInfo>> imageInfos;
  for (int i = 0; i < count; i++) {
    auto &desc = descriptor[i];
    if (std::holds_alternative<APITest::UniformBufferRef>(desc.descriptor)) {
      auto uniformBuffer = std::get<APITest::UniformBufferRef>(desc.descriptor);
      auto &bufferInfo =
          bufferInfos.emplace_back(std::make_unique<VkDescriptorBufferInfo>());
      auto *bufferImpl =
          dynamic_cast<VulkanUniformBuffer *>(uniformBuffer.get());
      if (!bufferImpl)
        throw std::runtime_error("[VULKAN][ERROR]: Invalid UniformBuffer "
                                 "object passed to descriptor set");
      bufferInfo->buffer = bufferImpl->buffer();
      bufferInfo->offset = 0;
      bufferInfo->range = bufferImpl->size();
      VkWriteDescriptorSet write = initializers::writeDescriptorSet(
          descriptorSet_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, desc.binding,
          bufferInfo.get());
      writes.push_back(write);
    } else if (std::holds_alternative<APITest::CombinedImageSampler>(
                   desc.descriptor)) {
      auto combinedImageSampler =
          std::get<APITest::CombinedImageSampler>(desc.descriptor);
      auto &imageInfo =
          imageInfos.emplace_back(std::make_unique<VkDescriptorImageInfo>());
      imageInfo->sampler =
          dynamic_cast<VulkanSampler *>(combinedImageSampler.first.get())
              ->get();
      imageInfo->imageView = dynamic_cast<VulkanImageInterface *>(
                                 combinedImageSampler.second.get())
                                 ->getDefaultView();
      // if we sample from image, assume we did our best to convert it to this
      // layout to the point
      imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      VkWriteDescriptorSet write = initializers::writeDescriptorSet(
          descriptorSet_, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          desc.binding, imageInfo.get());
      writes.push_back(write);
    }
  }

  vkUpdateDescriptorSets(parent->parent_->getVulkanDevice()->get(),
                         writes.size(), writes.data(), 0, nullptr);
}

VulkanDescriptorPool::VulkanDescriptorPool(
    VulkanRenderImpl *parent, const std::vector<VkDescriptorPoolSize> &sizes,
    int maxSets)
    : parent_(parent), maxSets_(maxSets) {
  descriptors.resize(sizes.size());
  std::transform(
      sizes.begin(), sizes.end(), descriptors.begin(),
      [](VkDescriptorPoolSize size) {
        return std::pair<VkDescriptorType, std::pair<size_t, size_t>>{
            size.type, std::pair<size_t, size_t>{size.descriptorCount, 0}};
      });
  VkDescriptorPoolCreateInfo createInfo =
      initializers::descriptorPoolCreateInfo(sizes, maxSets_);
  VK_CHECK_RESULT(vkCreateDescriptorPool(parent_->getVulkanDevice()->get(),
                                         &createInfo, nullptr, &pool))
}

VulkanDescriptorPool::~VulkanDescriptorPool() {
  vkDestroyDescriptorPool(parent_->getVulkanDevice()->get(), pool, nullptr);
}

UniformDescriptorSetRef VulkanDescriptorPool::allocateDescriptorSet(
    VkDescriptorSetLayout layout, UniformDescriptor *descriptor, int count) {
  std::vector<VkDescriptorPoolSize> totalSizes;
  for (int i = 0; i < count; i++) {
    auto type = convertInterfaceDescriptorType(descriptor[i]);
    auto found = std::find_if(
        totalSizes.begin(), totalSizes.end(),
        [type](VkDescriptorPoolSize size) { return type == size.type; });
    if (found != totalSizes.end())
      found->descriptorCount++;
    else
      totalSizes.emplace_back(VkDescriptorPoolSize{type, 1});
  }

  for (auto &descTypeCount : descriptors) {
    auto found = std::find_if(totalSizes.begin(), totalSizes.end(),
                              [descTypeCount](VkDescriptorPoolSize size) {
                                return descTypeCount.first == size.type;
                              });
    if (found == totalSizes.end())
      continue;

    if (found->descriptorCount + descTypeCount.second.second >
        descTypeCount.second.first)
      return nullptr;
    descTypeCount.second.second += found->descriptorCount;
  }

  allocatedSets.emplace_back(UniformDescriptorSetRef(
      new VulkanDescriptorSet(this, layout, descriptor, count)));
  return allocatedSets.back();
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
    VulkanDescriptorManager *parent, const std::vector<DescriptorLayout> &desc)
    : parent_(parent) {
  std::vector<VkDescriptorSetLayoutBinding> bindings;
  for (auto &layout : desc) {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = layout.binding;
    binding.descriptorType = convertInterfaceDescriptorType(layout.type);
    binding.stageFlags = convertStageFlags(layout.stage);
    binding.descriptorCount = 1;
    bindings.emplace_back(binding);
  }
  VkDescriptorSetLayoutCreateInfo createInfo =
      initializers::descriptorSetLayoutCreateInfo(bindings);

  VK_CHECK_RESULT(
      vkCreateDescriptorSetLayout(parent_->parent_->getVulkanDevice()->get(),
                                  &createInfo, nullptr, &setLayout_))
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
  vkDestroyDescriptorSetLayout(parent_->parent_->getVulkanDevice()->get(),
                               setLayout_, nullptr);
}

UniformDescriptorSetRef
VulkanDescriptorSetLayout::allocateNewSet(UniformDescriptor *descriptors,
                                          int count) {
  return parent_->allocateDescriptorSet(setLayout_, descriptors, count);
}
} // namespace APITest

APITest::UniformDescriptorSetRef
APITest::VulkanDescriptorManager::allocateDescriptorSet(
    VkDescriptorSetLayout layout, APITest::UniformDescriptor *descriptor,
    int count) {
  UniformDescriptorSetRef ret;

  for (auto &pool : pools_) {
    ret = pool->allocateDescriptorSet(layout, descriptor, count);
    if (ret)
      return std::move(ret);
  }
  std::vector<VkDescriptorPoolSize> sizes;
  for (int i = 0; i < count; i++) {
    auto type = convertInterfaceDescriptorType(descriptor[i]);
    auto size = std::find_if(sizes.begin(), sizes.end(),
                             [type](VkDescriptorPoolSize const &poolSize) {
                               return poolSize.type == type;
                             });
    if (size != sizes.end())
      size->descriptorCount++;
    else
      sizes.emplace_back(VkDescriptorPoolSize{type, 1});
  }

  // for now we will try to create new pool for 100 such descriptor sets if
  // count is low(less than 10) and for 1000 if above

  int allocateForSets = count > 10 ? 100 : 1000;

  for (auto &size : sizes)
    size.descriptorCount *= allocateForSets;

  pools_.emplace_back(
      std::make_unique<VulkanDescriptorPool>(parent_, sizes, allocateForSets));

  ret = pools_.back()->allocateDescriptorSet(layout, descriptor, count);
  if (!ret)
    throw std::runtime_error(
        "[VULKAN][ERROR]: cannot allocate descriptor pool.");
  return ret;
}
