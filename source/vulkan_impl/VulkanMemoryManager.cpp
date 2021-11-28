//
// Created by Бушев Дмитрий on 11.11.2021.
//

#include "VulkanMemoryManager.h"
#include "VulkanDevice.h"
#include "VulkanQueueManager.h"
#include "VulkanRenderImpl.h"
#include "util/VulkanInitializers.h"
#include <cassert>
#include <cstring>
#include <vulkan_impl/util/macro.h>

APITest::VulkanBuffer::~VulkanBuffer() {
  if (memoryManager != VK_NULL_HANDLE) {
    vmaDestroyBuffer(memoryManager->allocator(), buffer_, allocation);
  }
}
namespace APITest {
static VkImageUsageFlags convertInterfaceImageUsageFlags(Image::Usage usage) {
  VkImageUsageFlags ret = 0;

  if (usage & Image::USAGE_COPY_TO)
    ret |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  if (usage & Image::USAGE_COPY_FROM)
    ret |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  if (usage & Image::USAGE_SAMPLED)
    ret |= VK_IMAGE_USAGE_SAMPLED_BIT;
  if (usage & Image::USAGE_INPUT_ATTACHMENT)
    ret |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
  if (usage & Image::USAGE_DEPTH_STENCIL_ATTACHMENT)
    ret |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  if (usage & Image::USAGE_COLOR_ATTACHMENT)
    ret |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  return ret;
}
static VmaAllocationCreateInfo chooseMemoryOptions(MemoryType type) {
  VmaAllocationCreateInfo allocInfo = {};
  allocInfo.usage = type == MemoryType::GPU_PRIVATE ? VMA_MEMORY_USAGE_GPU_ONLY
                    : type == MemoryType::HOST_VISIBLE
                        ? VMA_MEMORY_USAGE_CPU_TO_GPU
                        : VMA_MEMORY_USAGE_CPU_ONLY;
  allocInfo.preferredFlags = type == MemoryType::HOST_COHERENT
                                 ? (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                    VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
                                 : 0;
  allocInfo.requiredFlags =
      type == MemoryType::HOST_VISIBLE || type == MemoryType::HOST_COHERENT
          ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
          : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  allocInfo.flags =
      type == MemoryType::HOST_VISIBLE || type == MemoryType::HOST_COHERENT
          ? VMA_ALLOCATION_CREATE_MAPPED_BIT
          : 0;

  return allocInfo;
}

VkImageCreateInfo
compileDepthStencilImageCreateInfo(VulkanDepthBufferCI createInfo) {
  VkImageCreateInfo ret = initializers::imageCreateInfo();
  ret.format = createInfo.depthFormat;
  ret.imageType = VK_IMAGE_TYPE_2D;
  ret.usage =
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | createInfo.additionalUsage;
  ret.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ret.samples = VK_SAMPLE_COUNT_1_BIT;
  ret.mipLevels = 1;
  ret.arrayLayers = 1;
  ret.tiling = createInfo.additionalUsage == 0 ? VK_IMAGE_TILING_OPTIMAL
                                               : VK_IMAGE_TILING_LINEAR;
  ret.extent = {createInfo.extents.width, createInfo.extents.height, 1};
  ret.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  return ret;
}
VulkanDepthBuffer::VulkanDepthBuffer(const VulkanMemoryManager *alloc,
                                     VulkanDepthBufferCI createInfo)
    : VulkanImage(
          alloc, MemoryType::GPU_PRIVATE,
          static_cast<VkBufferUsageFlagBits>(
              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
              convertInterfaceImageUsageFlags(createInfo.additionalUsage)),
          compileDepthStencilImageCreateInfo(createInfo)) {
  VkImageViewCreateInfo viewInfo = initializers::imageViewCreateInfo();
  viewInfo.format = createInfo.depthFormat;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.image = image();
  viewInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                         VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
  viewInfo.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

  VK_CHECK_RESULT(
      vkCreateImageView(alloc->device(), &viewInfo, nullptr, &depthView_))
}

VulkanDepthBuffer::~VulkanDepthBuffer() {
  if (depthView_ != VK_NULL_HANDLE) {
    vkDestroyImageView(memoryManager->device(), depthView_, nullptr);
  }
}

VkImageView VulkanDepthBuffer::getDepthView() const { return depthView_; }

Image::Type VulkanImageInterface::getType() const {
  switch (imageInfo_.imageType) {
  case VK_IMAGE_TYPE_1D:
    return Image::Type::TEXTURE_1D;
  case VK_IMAGE_TYPE_2D:
    return Image::Type::TEXTURE_2D;
  case VK_IMAGE_TYPE_3D:
    return Image::Type::TEXTURE_3D;
  default:
    return Image::Type::INVALID;
  }
}

Image::Format VulkanImageInterface::getFormat() const {
  switch (imageInfo_.format) {
  case VK_FORMAT_R8G8B8A8_SRGB:
    return Image::Format::RGBA8;
  case VK_FORMAT_D24_UNORM_S8_UINT:
    return Image::Format::D24S8;
  case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
    return Image::Format::R11G11B10;
  default:
    return Image::Format::INVALID;
  }
}

Image::Extents VulkanImageInterface::getImageExtents() const {
  return {imageInfo_.extent.width, imageInfo_.extent.height,
          imageInfo_.extent.depth};
}

uint32_t VulkanImageInterface::arrayLayers() const {
  return imageInfo_.arrayLayers;
}

Image::Usage VulkanImageInterface::getUsage() const {
  Image::Usage ret;
  if (imageInfo_.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    ret |= Image::USAGE_COLOR_ATTACHMENT;
  if (imageInfo_.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
    ret |= Image::USAGE_DEPTH_STENCIL_ATTACHMENT;
  if (imageInfo_.usage & VK_IMAGE_USAGE_SAMPLED_BIT)
    ret |= Image::USAGE_SAMPLED;
  if (imageInfo_.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    ret |= Image::USAGE_COPY_TO;
  if (imageInfo_.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    ret |= Image::USAGE_COPY_FROM;
  if (imageInfo_.usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
    ret |= Image::USAGE_INPUT_ATTACHMENT;

  return ret;
}

void VulkanImageInterface::load(const void *data) {

  if (data == nullptr)
    throw std::runtime_error(
        "[VULKAN][ERROR]: passed nullptr to image->load().");

  size_t imageSize = imageInfo_.extent.width * imageInfo_.extent.height * 4;

  VulkanStagingBuffer buffer{memoryManager, imageSize};
  buffer.push(data, imageSize, 0);

  memoryManager->transitLayout(this, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  memoryManager->copy(&buffer, this);

  memoryManager->transitLayout(this, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  currentLayout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

static VkImageAspectFlagBits getAspect(VkFormat format) {
  if (format == VK_FORMAT_D24_UNORM_S8_UINT || format == VK_FORMAT_D16_UNORM ||
      format == VK_FORMAT_D16_UNORM_S8_UINT || format == VK_FORMAT_D32_SFLOAT ||
      format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
    int ret;
    ret = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (format >= VK_FORMAT_D16_UNORM_S8_UINT) {
      ret |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    return static_cast<VkImageAspectFlagBits>(ret);
  } else {
    return VK_IMAGE_ASPECT_COLOR_BIT;
  }
}

void VulkanImageInterface::createDefaultImageView() {
  VkImageViewCreateInfo createInfo = initializers::imageViewCreateInfo();
  createInfo.image = image();
  createInfo.format = imageInfo_.format;
  switch (imageInfo_.imageType) {
  case VK_IMAGE_TYPE_1D:
    createInfo.viewType = imageInfo_.arrayLayers > 1
                              ? VK_IMAGE_VIEW_TYPE_1D_ARRAY
                              : VK_IMAGE_VIEW_TYPE_1D;
    break;
  case VK_IMAGE_TYPE_2D:
    createInfo.viewType = imageInfo_.arrayLayers > 1
                              ? VK_IMAGE_VIEW_TYPE_2D_ARRAY
                              : VK_IMAGE_VIEW_TYPE_2D;
    break;
  case VK_IMAGE_TYPE_3D:
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
    break;
  }

  createInfo.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                           VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
  createInfo.subresourceRange = {
      (VkImageAspectFlags)getAspect(createInfo.format), 0, 1, 0, 1};

  VK_CHECK_RESULT(vkCreateImageView(memoryManager->device(), &createInfo,
                                    nullptr, &defaultImageView_))
}

VulkanImageInterface::~VulkanImageInterface() {
  vkDestroyImageView(memoryManager->device(), defaultImageView_, nullptr);
}

VkFormat convertInterfaceImageFormat(Image::Format format) {
  switch (format) {
  case Image::Format::RGBA8:
    return VK_FORMAT_R8G8B8A8_UNORM;
  case Image::Format::R11G11B10:
    return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
  case Image::Format::D24S8:
    return VK_FORMAT_D24_UNORM_S8_UINT;
  default:
    return VK_FORMAT_UNDEFINED;
  }
}
VkImageType convertInterfaceImageType(Image::Type type) {
  switch (type) {
  case Image::Type::TEXTURE_1D:
    return VK_IMAGE_TYPE_1D;
  case Image::Type::TEXTURE_2D:
    return VK_IMAGE_TYPE_2D;
  case Image::Type::TEXTURE_3D:
    return VK_IMAGE_TYPE_3D;
  default:
    return VK_IMAGE_TYPE_MAX_ENUM;
  }
}
VkImageCreateInfo compileGeneralVulkanImageCI(ImageDesc desc) {
  VkImageCreateInfo ret = initializers::imageCreateInfo();

  ret.usage = convertInterfaceImageUsageFlags(desc.usage);
  ret.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ret.format = convertInterfaceImageFormat(desc.format);

  if (desc.type != Image::Type::TEXTURE_3D && desc.extents.depth != 1)
    throw std::runtime_error(
        "[VULKAN][ERROR] if Image::Type != TEXTURE_3D depth must be equal 1");
  if (desc.type == Image::Type::TEXTURE_1D && desc.extents.height != 1)
    throw std::runtime_error(
        "[VULKAN][ERROR] if Image::Type == TEXTURE_1D height must be equal 1");

  ret.extent = {desc.extents.width, desc.extents.height, desc.extents.depth};
  ret.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ret.arrayLayers = desc.extents.layers;
  ret.samples = VK_SAMPLE_COUNT_1_BIT;
  ret.imageType = convertInterfaceImageType(desc.type);
  ret.tiling = desc.memType == MemoryType::GPU_PRIVATE ? VK_IMAGE_TILING_OPTIMAL
                                                       : VK_IMAGE_TILING_LINEAR;
  ret.flags = 0;
  ret.mipLevels = 1;
  return ret;
}

VulkanGeneralImage::VulkanGeneralImage(VulkanMemoryManager const *alloc,
                                       ImageDesc desc)
    : VulkanImage(alloc, desc.memType,
                  static_cast<VkBufferUsageFlagBits>(
                      convertInterfaceImageUsageFlags(desc.usage)),
                  compileGeneralVulkanImageCI(desc)) {}

VulkanStagingBuffer::VulkanStagingBuffer(const VulkanMemoryManager *alloc,
                                         size_t size)
    : VulkanBuffer(alloc, MemoryType::HOST_COHERENT,
                   VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size) {}
} // namespace APITest

APITest::VulkanBuffer::VulkanBuffer(VulkanMemoryManager const *alloc,
                                    APITest::MemoryType type,
                                    VkBufferUsageFlagBits usage, size_t size)
    : Allocatable(alloc), usage_(usage) {

  if (size == 0)
    throw std::runtime_error("[VULKAN][ERROR] trying to create buffer with "
                             "size == 0 which is prohibited.");

  size_ = size;
  // If allocating in private memory, pushing data will involve explicit data
  // transfer operation
  if (type == MemoryType::GPU_PRIVATE)
    usage_ |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

  VkBufferCreateInfo bufferCI = initializers::bufferCreateInfo(usage_, size);
  bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo = chooseMemoryOptions(type);

  vmaCreateBuffer(memoryManager->allocator(), &bufferCI, &allocInfo, &buffer_,
                  &allocation, &allocationInfo);
}

void APITest::VulkanBuffer::push(const void *data, size_t size, size_t offset) {

  if (size + offset > allocationInfo.size)
    throw std::runtime_error(
        "[VULKAN][ERROR] pushing to out of bounds memory. " +
        std::to_string(size + offset - (allocationInfo.size)) +
        " bytes off bounds");

  const VkPhysicalDeviceMemoryProperties *pMemProps;
  vmaGetMemoryProperties(memoryManager->allocator(), &pMemProps);
  auto bits = pMemProps->memoryTypes[allocationInfo.memoryType].propertyFlags;

  if (bits & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ||
      bits & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
    if (allocationInfo.pMappedData) {
      memcpy((char *)allocationInfo.pMappedData + offset, data, size);
    } else {
      void *mapped;
      vmaMapMemory(memoryManager->allocator(), allocation, &mapped);

      memcpy((char *)mapped + offset, data, size);

      vmaUnmapMemory(memoryManager->allocator(), allocation);
    }
  } else {
    VkBufferCreateInfo bufferCI =
        initializers::bufferCreateInfo(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size);
    bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    allocInfo.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                               VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VmaAllocationInfo stagingBufferInfo;
    VmaAllocation stagingAllocation;
    VkBuffer stagingBuffer;

    vmaCreateBuffer(memoryManager->allocator(), &bufferCI, &allocInfo,
                    &stagingBuffer, &stagingAllocation, &stagingBufferInfo);

    auto mapped = stagingBufferInfo.pMappedData;

    memcpy((char *)mapped, data, size);

    VkBufferCopy copyRegion;
    copyRegion.size = size;
    copyRegion.dstOffset = offset;
    copyRegion.srcOffset = 0;
    memoryManager->copy(stagingBuffer, buffer_, copyRegion);

    vmaDestroyBuffer(memoryManager->allocator(), stagingBuffer,
                     stagingAllocation);
  }
}

APITest::VulkanMemoryManager::VulkanMemoryManager(
    APITest::VulkanRenderImpl *parent)
    : parent_(parent) {
  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
  allocatorInfo.physicalDevice = parent_->getVulkanDevice()->getPhysical();
  allocatorInfo.device = parent_->getVulkanDevice()->get();
  allocatorInfo.instance = parent_->getInstance();

  VK_CHECK_RESULT(vmaCreateAllocator(&allocatorInfo, &allocator_))
}

void APITest::VulkanMemoryManager::copy(VkBuffer src, VkBuffer dst,
                                        VkBufferCopy region) const {
  parent_->getQueueManager()->copyBuffer(src, dst, region);
}

APITest::VulkanMemoryManager::~VulkanMemoryManager() {
  vmaDestroyAllocator(allocator_);
}

VkDevice APITest::VulkanMemoryManager::device() const {
  return parent_->getVulkanDevice()->get();
}

void APITest::VulkanMemoryManager::transitLayout(
    APITest::VulkanImage *image, VkImageLayout newLayout) const {
  VkCommandBuffer transitCmd;
  parent_->getQueueManager()->createPrimaryCommandBuffers(&transitCmd, 1, true);

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = image->currentLayout();
  barrier.newLayout = newLayout;

  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  barrier.image = image->image();
  barrier.subresourceRange.aspectMask = getAspect(image->format());
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  auto oldLayout = image->currentLayout();

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

  } else {
    assert(0 && "This image transition layout isn't supported yet");
  }

  vkCmdPipelineBarrier(transitCmd, sourceStage /* TODO */,
                       destinationStage /* TODO */, 0, 0, nullptr, 0, nullptr,
                       1, &barrier);

  parent_->getQueueManager()->flush(
      transitCmd, parent_->getQueueManager()->getTransferQueue(), true);
}

void APITest::VulkanMemoryManager::copy(APITest::VulkanBuffer *buffer,
                                        APITest::VulkanImage *image) const {
  VkCommandBuffer copyCmd;
  parent_->getQueueManager()->createPrimaryCommandBuffers(&copyCmd, 1, true);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = getAspect(image->format());
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = image->extents();

  vkCmdCopyBufferToImage(copyCmd, buffer->buffer(), image->image(),
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  parent_->getQueueManager()->flush(
      copyCmd, parent_->getQueueManager()->getTransferQueue(), true);
}

APITest::VulkanImage::VulkanImage(const APITest::VulkanMemoryManager *alloc,
                                  APITest::MemoryType type,
                                  VkBufferUsageFlagBits usage,
                                  VkImageCreateInfo CI)
    : Allocatable(alloc), usage_(usage), imageInfo_(CI),
      currentLayout_(CI.initialLayout) {

  VmaAllocationCreateInfo allocInfo = chooseMemoryOptions(type);

  VK_CHECK_RESULT(vmaCreateImage(memoryManager->allocator(), &CI, &allocInfo,
                                 &image_, &allocation, &allocationInfo))
}

APITest::VulkanImage::~VulkanImage() {
  vmaDestroyImage(memoryManager->allocator(), image_, allocation);
}
