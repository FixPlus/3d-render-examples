//
// Created by Бушев Дмитрий on 11.11.2021.
//

#ifndef RENDERAPITEST_VULKANMEMORYMANAGER_H
#define RENDERAPITEST_VULKANMEMORYMANAGER_H
#include <RenderInterface.h>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace APITest {

class VulkanRenderImpl;
class VulkanMemoryManager;

struct Allocatable {
protected:
  VulkanMemoryManager const *memoryManager;
  VmaAllocation allocation = VK_NULL_HANDLE;
  VmaAllocationInfo allocationInfo;
  size_t size_ = 0;

public:
  explicit Allocatable(VulkanMemoryManager const *alloc)
      : memoryManager(alloc) {}
  virtual ~Allocatable() = default;
};

class VulkanBuffer : virtual public BufferBase, public Allocatable {
  VkBuffer buffer_ = VK_NULL_HANDLE;
  VkBufferUsageFlags usage_;

public:
  VulkanBuffer(VulkanMemoryManager const *alloc, MemoryType type,
               VkBufferUsageFlagBits usage, size_t size);
  VkBuffer buffer() const { return buffer_; };
  void push(const void *data, size_t size, size_t offset) override;
  size_t size() const override { return size_; }
  ~VulkanBuffer() override;
};

struct VulkanVertexBuffer : public VertexBuffer, public VulkanBuffer {
  VulkanVertexBuffer(VulkanMemoryManager const *alloc, MemoryType type,
                     size_t size)
      : VulkanBuffer(alloc, type, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size){};
};

struct VulkanIndexBuffer : public IndexBuffer, public VulkanBuffer {
  IndexBuffer::Type type;
  VulkanIndexBuffer(VulkanMemoryManager const *alloc, MemoryType type,
                    size_t size, IndexBuffer::Type indType)
      : VulkanBuffer(alloc, type, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size),
        type(indType) {}
};
struct VulkanStagingBuffer : public VulkanBuffer {
  VulkanStagingBuffer(VulkanMemoryManager const *alloc, size_t size);
};

struct VulkanUniformBuffer : public UniformBuffer, public VulkanBuffer {
  VulkanUniformBuffer(VulkanMemoryManager const *alloc, MemoryType type,
                      size_t size)
      : VulkanBuffer(alloc, type, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size){};
};

class VulkanImage : public Allocatable {
  VkImage image_ = VK_NULL_HANDLE;
  VkImageUsageFlags usage_;

protected:
  VkImageCreateInfo imageInfo_;
  // this is layout in which image should enter and leave frame render process
  VkImageLayout currentLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;

public:
  VulkanImage(VulkanMemoryManager const *alloc, MemoryType type,
              VkBufferUsageFlagBits usage, VkImageCreateInfo CI);

  VkImage image() const { return image_; }
  VkFormat format() const { return imageInfo_.format; }
  VkImageUsageFlags usage() const { return usage_; }
  VkImageLayout currentLayout() const { return currentLayout_; }
  VkExtent3D extents() const { return imageInfo_.extent; }
  ~VulkanImage() override;
};

class VulkanImageInterface : public Image, virtual public VulkanImage {
  VkImageView defaultImageView_ = VK_NULL_HANDLE;
  void createDefaultImageView();

public:
  VulkanImageInterface()
      : VulkanImage(nullptr, MemoryType::GPU_PRIVATE,
                    static_cast<VkBufferUsageFlagBits>(
                        VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM),
                    VkImageCreateInfo{}) {
    createDefaultImageView();
  }
  void load(void const *data) override;
  Type getType() const override;
  Format getFormat() const override;
  Extents getImageExtents() const override;
  uint32_t arrayLayers() const override;
  Usage getUsage() const override;
  VkImageView getDefaultView() const { return defaultImageView_; };

  ~VulkanImageInterface() override;
};

class VulkanGeneralImage : public VulkanImageInterface,
                           virtual public VulkanImage {
public:
  VulkanGeneralImage(VulkanMemoryManager const *alloc, ImageDesc desc);
};

struct VulkanDepthBufferCI {
  VkFormat depthFormat;
  VkExtent2D extents;
  Image::Usage additionalUsage; // maybe sample from it
};

class VulkanDepthBuffer final : public VulkanImageInterface,
                                virtual public VulkanImage {
  VkImageView depthView_ = VK_NULL_HANDLE;

public:
  VulkanDepthBuffer(VulkanMemoryManager const *alloc,
                    VulkanDepthBufferCI createInfo);
  VkImageView getDepthView() const;
  ~VulkanDepthBuffer() override;
};

class VulkanMemoryManager {
  VulkanRenderImpl *parent_ = nullptr;
  VmaAllocator allocator_ = VK_NULL_HANDLE;

public:
  VmaAllocator allocator() const { return allocator_; }
  VkDevice device() const;
  void copy(VkBuffer src, VkBuffer dst, VkBufferCopy region) const;
  void transitLayout(VulkanImage *image, VkImageLayout newLayout) const;
  void copy(VulkanBuffer *buffer, VulkanImage *image) const;
  explicit VulkanMemoryManager(VulkanRenderImpl *parent);
  ~VulkanMemoryManager();
};
} // namespace APITest
#endif // RENDERAPITEST_VULKANMEMORYMANAGER_H
