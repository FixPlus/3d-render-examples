//
// Created by Бушев Дмитрий on 28.10.2021.
//

#ifndef RENDERAPITEST_VULKANSWAPCHAIN_H
#define RENDERAPITEST_VULKANSWAPCHAIN_H
#include <vector>
#include <vulkan/vulkan.h>

namespace APITest {

class VulkanRenderImpl;
class VulkanDevice;

typedef struct _SwapChainBuffers {
  VkImage image;
  VkImageView view;
} SwapChainBuffer;

class VulkanSwapChain final {

  VulkanRenderImpl *parent_;
  VkSurfaceKHR surface_;

  // Function pointers
  PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
      fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
  PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
      fpGetPhysicalDeviceSurfacePresentModesKHR;
  PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
  PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
  PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
  PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
  PFN_vkQueuePresentKHR fpQueuePresentKHR;

  struct Sync {
    uint32_t framesInFlight = 1;
    uint32_t currentFrame = 0;
    uint32_t currentImage = 0;
    std::vector<VkFence> swapChainFences;
    struct PerFrame {
      // Swap chain image presentation
      VkSemaphore presentComplete;
      // Command buffer submission and execution
      VkSemaphore renderComplete;

      VkFence inFlightSync;
    };
    std::vector<PerFrame> frames;

    const VkPipelineStageFlags submitPipelineStages =
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  } sync;

  VkSubmitInfo submitInfo;

public:
  uint32_t surfaceWidth, surfaceHeight;
  VkFormat colorFormat;
  VkColorSpaceKHR colorSpace;
  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  uint32_t imageCount;
  std::vector<VkImage> images;
  std::vector<SwapChainBuffer> buffers;

  VulkanSwapChain(VulkanRenderImpl *parent_, VkSurfaceKHR surface);

  void recreate(uint32_t *width, uint32_t *height, bool vsync = false);

  VkResult acquireNextImage(uint32_t *imageIndex);
  VkResult submitAndPresent(VkCommandBuffer *cmd);

  ~VulkanSwapChain();
};
} // namespace APITest

#endif // RENDERAPITEST_VULKANSWAPCHAIN_H
