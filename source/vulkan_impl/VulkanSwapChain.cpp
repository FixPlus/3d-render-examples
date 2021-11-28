//
// Created by Бушев Дмитрий on 28.10.2021.
//

#include "VulkanSwapChain.h"
#include "vulkan_impl/VulkanDevice.h"
#include "vulkan_impl/VulkanQueueManager.h"
#include "vulkan_impl/VulkanRenderImpl.h"
#include "vulkan_impl/util/VulkanInitializers.h"
#include "vulkan_impl/util/macro.h"
#include <cassert>
#include <iostream>

APITest::VulkanSwapChain::VulkanSwapChain(APITest::VulkanRenderImpl *parent,
                                          VkSurfaceKHR surface)
    : parent_(parent), surface_(surface) {

  fpGetPhysicalDeviceSurfaceSupportKHR =
      reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(
          vkGetInstanceProcAddr(parent_->getInstance(),
                                "vkGetPhysicalDeviceSurfaceSupportKHR"));
  fpGetPhysicalDeviceSurfaceCapabilitiesKHR =
      reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(
          vkGetInstanceProcAddr(parent_->getInstance(),
                                "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
  fpGetPhysicalDeviceSurfaceFormatsKHR =
      reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(
          vkGetInstanceProcAddr(parent_->getInstance(),
                                "vkGetPhysicalDeviceSurfaceFormatsKHR"));
  fpGetPhysicalDeviceSurfacePresentModesKHR =
      reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(
          vkGetInstanceProcAddr(parent_->getInstance(),
                                "vkGetPhysicalDeviceSurfacePresentModesKHR"));

  auto device = parent_->getVulkanDevice();

  fpCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(
      vkGetDeviceProcAddr(device->get(), "vkCreateSwapchainKHR"));
  fpDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(
      vkGetDeviceProcAddr(device->get(), "vkDestroySwapchainKHR"));
  fpGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(
      vkGetDeviceProcAddr(device->get(), "vkGetSwapchainImagesKHR"));
  fpAcquireNextImageKHR = reinterpret_cast<PFN_vkAcquireNextImageKHR>(
      vkGetDeviceProcAddr(device->get(), "vkAcquireNextImageKHR"));
  fpQueuePresentKHR = reinterpret_cast<PFN_vkQueuePresentKHR>(
      vkGetDeviceProcAddr(device->get(), "vkQueuePresentKHR"));

  // Get available queue family properties
  uint32_t queueCount;
  auto physicalDevice = parent_->getVulkanDevice()->getPhysical();
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
  assert(queueCount >= 1);

  std::vector<VkQueueFamilyProperties> queueProps(queueCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount,
                                           queueProps.data());

  // Iterate over each queue to learn whether it supports presenting:
  // Find a queue with present support
  // Will be used to present the swap chain images to the windowing system
  std::vector<VkBool32> supportsPresent(queueCount);
  for (uint32_t i = 0; i < queueCount; i++) {
    fpGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                         &supportsPresent[i]);
  }

  // Search for a graphics and a present queue in the array of queue
  // families, try to find one that supports both
  uint32_t graphicsQueueNodeIndex = UINT32_MAX;
  uint32_t presentQueueNodeIndex = UINT32_MAX;
  for (uint32_t i = 0; i < queueCount; i++) {
    if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
      if (graphicsQueueNodeIndex == UINT32_MAX) {
        graphicsQueueNodeIndex = i;
      }

      if (supportsPresent[i] == VK_TRUE) {
        graphicsQueueNodeIndex = i;
        presentQueueNodeIndex = i;
        break;
      }
    }
  }
  if (presentQueueNodeIndex == UINT32_MAX) {
    // If there's no queue that supports both present and graphics
    // try to find a separate present queue
    for (uint32_t i = 0; i < queueCount; ++i) {
      if (supportsPresent[i] == VK_TRUE) {
        presentQueueNodeIndex = i;
        break;
      }
    }
  }

  // Exit if either a graphics or a presenting queue hasn't been found
  if (graphicsQueueNodeIndex == UINT32_MAX ||
      presentQueueNodeIndex == UINT32_MAX) {
    throw std::runtime_error(
        "Could not find a graphics and/or presenting queue!");
  }

  // todo : Add support for separate graphics and presenting queue
  if (graphicsQueueNodeIndex != presentQueueNodeIndex) {
    throw std::runtime_error(
        "Separate graphics and presenting queues are not supported yet!");
  }

  // Get list of supported surface formats
  uint32_t formatCount;
  if (auto err = (fpGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                                       &formatCount, NULL) -
                  VK_SUCCESS))
    throw std::runtime_error("Failed to query surface format. Err code: " +
                             std::to_string(err + VK_SUCCESS));
  assert(formatCount > 0);

  std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
  if (auto err =
          (fpGetPhysicalDeviceSurfaceFormatsKHR(
               physicalDevice, surface, &formatCount, surfaceFormats.data()) -
           VK_SUCCESS))
    throw std::runtime_error("Failed to query surface format. Err code: " +
                             std::to_string(err + VK_SUCCESS));

  // If the surface format list only includes one entry with
  // VK_FORMAT_UNDEFINED, there is no preferred format, so we assume
  // VK_FORMAT_B8G8R8A8_UNORM
  if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED)) {
    colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    colorSpace = surfaceFormats[0].colorSpace;
  } else {
    // iterate over the list of available surface format and
    // check for the presence of VK_FORMAT_B8G8R8A8_UNORM
    bool found_B8G8R8A8_UNORM = false;
    for (auto &&surfaceFormat : surfaceFormats) {
      if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM) {
        colorFormat = surfaceFormat.format;
        colorSpace = surfaceFormat.colorSpace;
        found_B8G8R8A8_UNORM = true;
        break;
      }
    }

    // in case VK_FORMAT_B8G8R8A8_UNORM is not available
    // select the first available color format
    if (!found_B8G8R8A8_UNORM) {
      colorFormat = surfaceFormats[0].format;
      colorSpace = surfaceFormats[0].colorSpace;
    }
  }

  // create synchronisation primitives

  sync.frames.resize(sync.framesInFlight);

  VkSemaphoreCreateInfo semaphoreCreateInfo =
      initializers::semaphoreCreateInfo();

  VkFenceCreateInfo fenceCreateInfo = initializers::fenceCreateInfo();
  fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (auto &frameSync : sync.frames) {
    // Create a semaphore used to synchronize image presentation
    // Ensures that the image is displayed before we start submitting new
    // commands to the queue
    VK_CHECK_RESULT(vkCreateSemaphore(device->get(), &semaphoreCreateInfo,
                                      nullptr, &frameSync.presentComplete));
    // Create a semaphore used to synchronize command submission
    // Ensures that the image is not presented until all commands have been
    // submitted and executed
    VK_CHECK_RESULT(vkCreateSemaphore(device->get(), &semaphoreCreateInfo,
                                      nullptr, &frameSync.renderComplete));

    VK_CHECK_RESULT(vkCreateFence(device->get(), &fenceCreateInfo, nullptr,
                                  &frameSync.inFlightSync));
  }

  // currentFrame = 0;

  // Set up submit info structure
  // Semaphores will stay the same during application lifetime
  // Command buffer submission info is set by each example
  submitInfo = initializers::submitInfo();
  submitInfo.pWaitDstStageMask = &sync.submitPipelineStages;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &sync.frames[0].presentComplete;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &sync.frames[0].renderComplete;
}

void APITest::VulkanSwapChain::recreate(uint32_t *width, uint32_t *height,
                                        bool vsync) {

  auto physicalDevice = parent_->getVulkanDevice()->getPhysical();
  auto device = parent_->getVulkanDevice()->get();

  VkSwapchainKHR oldSwapchain = swapChain;

  // Get physical device surface properties and formats
  VkSurfaceCapabilitiesKHR surfCaps;
  if (fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface_,
                                                &surfCaps) != VK_SUCCESS)
    throw std::runtime_error("");

  // Get available present modes
  uint32_t presentModeCount;
  VK_CHECK_RESULT(fpGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice, surface_, &presentModeCount, NULL));
  assert(presentModeCount > 0);

  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  VK_CHECK_RESULT(fpGetPhysicalDeviceSurfacePresentModesKHR(
      physicalDevice, surface_, &presentModeCount, presentModes.data()));

  VkExtent2D swapchainExtent = {};
  // If width (and height) equals the special value 0xFFFFFFFF, the size of the
  // surface will be set by the swapchain
  if (surfCaps.currentExtent.width == (uint32_t)-1) {
    // If the surface size is undefined, the size is set to
    // the size of the images requested.
    swapchainExtent.width = *width;
    swapchainExtent.height = *height;
  } else {
    // If the surface size is defined, the swap chain size must match
    swapchainExtent = surfCaps.currentExtent;
    *width = surfCaps.currentExtent.width;
    *height = surfCaps.currentExtent.height;
  }
  surfaceWidth = *width;
  surfaceHeight = *height;

  // Select a present mode for the swapchain

  // The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
  // This mode waits for the vertical blank ("v-sync")
  VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

  // If v-sync is not requested, try to find a mailbox mode
  // It's the lowest latency non-tearing present mode available
  if (!vsync) {
    for (size_t i = 0; i < presentModeCount; i++) {
      if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }
      if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) &&
          (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
        swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      }
    }
  }

  // Determine the number of images
  uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
  if ((surfCaps.maxImageCount > 0) &&
      (desiredNumberOfSwapchainImages > surfCaps.maxImageCount)) {
    desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
  }

  // Find the transformation of the surface
  VkSurfaceTransformFlagsKHR preTransform;
  if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    // We prefer a non-rotated transform
    preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    preTransform = surfCaps.currentTransform;
  }

  // Find a supported composite alpha format (not all devices support alpha
  // opaque)
  VkCompositeAlphaFlagBitsKHR compositeAlpha =
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  // Simply select the first composite alpha format available
  std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
  };
  for (auto &compositeAlphaFlag : compositeAlphaFlags) {
    if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
      compositeAlpha = compositeAlphaFlag;
      break;
    };
  }

  VkSwapchainCreateInfoKHR swapchainCI = {};
  swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchainCI.pNext = NULL;
  swapchainCI.surface = surface_;
  swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
  swapchainCI.imageFormat = colorFormat;
  swapchainCI.imageColorSpace = colorSpace;
  swapchainCI.imageExtent = {swapchainExtent.width, swapchainExtent.height};
  swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
  swapchainCI.imageArrayLayers = 1;
  swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainCI.queueFamilyIndexCount = 0;
  swapchainCI.pQueueFamilyIndices = NULL;
  swapchainCI.presentMode = swapchainPresentMode;
  swapchainCI.oldSwapchain = oldSwapchain;
  // Setting clipped to VK_TRUE allows the implementation to discard rendering
  // outside of the surface area
  swapchainCI.clipped = VK_TRUE;
  swapchainCI.compositeAlpha = compositeAlpha;

  // Enable transfer source on swap chain images if supported
  if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
    swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  // Enable transfer destination on swap chain images if supported
  if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
    swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  VK_CHECK_RESULT(
      fpCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapChain));

  // If an existing swap chain is re-created, destroy the old swap chain
  // This also cleans up all the presentable images
  if (oldSwapchain != VK_NULL_HANDLE) {
    for (uint32_t i = 0; i < imageCount; i++) {
      vkDestroyImageView(device, buffers[i].view, nullptr);
    }
    fpDestroySwapchainKHR(device, oldSwapchain, nullptr);
  }
  VK_CHECK_RESULT(
      fpGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL));

  // Get the swap chain images
  images.resize(imageCount);
  VK_CHECK_RESULT(
      fpGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

  // Get the swap chain buffers containing the image and imageview
  buffers.resize(imageCount);
  for (uint32_t i = 0; i < imageCount; i++) {
    VkImageViewCreateInfo colorAttachmentView = {};
    colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorAttachmentView.pNext = NULL;
    colorAttachmentView.format = colorFormat;
    colorAttachmentView.components = {
        VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A};
    colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorAttachmentView.subresourceRange.baseMipLevel = 0;
    colorAttachmentView.subresourceRange.levelCount = 1;
    colorAttachmentView.subresourceRange.baseArrayLayer = 0;
    colorAttachmentView.subresourceRange.layerCount = 1;
    colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorAttachmentView.flags = 0;

    buffers[i].image = images[i];

    colorAttachmentView.image = buffers[i].image;

    VK_CHECK_RESULT(vkCreateImageView(device, &colorAttachmentView, nullptr,
                                      &buffers[i].view))

    assert(sync.framesInFlight <= imageCount &&
           "FIF count must be less or equal to number of swap chain image "
           "buffers");
    sync.swapChainFences.resize(imageCount, VK_NULL_HANDLE);

    // TODO: remove this block of code when layout transition will be handled by
    // render pass

    VkCommandBuffer transitCmd;
    VK_CHECK_RESULT(
        parent_->getQueueManager()->createPrimaryCommandBuffers(&transitCmd, 1))
    VkCommandBufferBeginInfo cmdBufInfo =
        initializers::commandBufferBeginInfo();
    VK_CHECK_RESULT(vkBeginCommandBuffer(transitCmd, &cmdBufInfo))

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = images[i];
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    vkCmdPipelineBarrier(transitCmd, sourceStage /* TODO */,
                         destinationStage /* TODO */, 0, 0, nullptr, 0, nullptr,
                         1, &barrier);

    VK_CHECK_RESULT(vkEndCommandBuffer(transitCmd));

    VkSubmitInfo submitInfo = initializers::submitInfo();
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transitCmd;
    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo = initializers::fenceCreateInfo(VK_FLAGS_NONE);
    VkFence fence;
    VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence));
    // Submit to the queue
    VK_CHECK_RESULT(vkQueueSubmit(parent_->getQueueManager()->getMainQueue(), 1,
                                  &submitInfo, fence));
    // Wait for the fence to signal that command buffer has finished executing
    VK_CHECK_RESULT(
        vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));
    vkDestroyFence(device, fence, nullptr);

    // layout transition end
  }
}

APITest::VulkanSwapChain::~VulkanSwapChain() {

  auto device = parent_->getVulkanDevice()->get();

  for (auto &frameSync : sync.frames) {
    vkDestroySemaphore(device, frameSync.presentComplete, nullptr);
    vkDestroySemaphore(device, frameSync.renderComplete, nullptr);
    vkDestroyFence(device, frameSync.inFlightSync, nullptr);
  }

  if (swapChain != VK_NULL_HANDLE) {
    for (uint32_t i = 0; i < imageCount; i++) {
      vkDestroyImageView(device, buffers[i].view, nullptr);
    }
    fpDestroySwapchainKHR(device, swapChain, nullptr);
  }

  if (surface_ != VK_NULL_HANDLE)
    vkDestroySurfaceKHR(parent_->getInstance(), surface_, nullptr);
}

VkResult APITest::VulkanSwapChain::acquireNextImage(uint32_t *imageIndex) {

  vkWaitForFences(parent_->getVulkanDevice()->get(), 1,
                  &sync.frames[sync.currentFrame].inFlightSync, VK_TRUE,
                  UINT64_MAX);

  auto ret = fpAcquireNextImageKHR(
      parent_->getVulkanDevice()->get(), swapChain, UINT64_MAX,
      sync.frames[sync.currentFrame].presentComplete, (VkFence) nullptr,
      imageIndex);
  sync.currentImage = *imageIndex;
  return ret;
}

VkResult APITest::VulkanSwapChain::submitAndPresent(VkCommandBuffer *cmd) {

  auto device = parent_->getVulkanDevice();
  if (sync.swapChainFences[sync.currentImage] != VK_NULL_HANDLE) {
    vkWaitForFences(device->get(), 1, &sync.swapChainFences[sync.currentImage],
                    VK_TRUE, UINT64_MAX);
  }
  // Mark the image as now being in use by this frame
  sync.swapChainFences[sync.currentImage] =
      sync.frames[sync.currentFrame].inFlightSync;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = cmd;

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &sync.frames[sync.currentFrame].presentComplete;
  submitInfo.pSignalSemaphores = &sync.frames[sync.currentFrame].renderComplete;

  vkResetFences(device->get(), 1, &sync.frames[sync.currentFrame].inFlightSync);

  auto queue = parent_->getQueueManager()->getMainQueue();
  VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo,
                                sync.frames[sync.currentFrame].inFlightSync));

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = NULL;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapChain;
  presentInfo.pImageIndices = &sync.currentImage;
  presentInfo.pWaitSemaphores = &sync.frames[sync.currentFrame].renderComplete;
  presentInfo.waitSemaphoreCount = 1;

  auto result = fpQueuePresentKHR(queue, &presentInfo);

  if (((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
    // switch frame only if present successful
    sync.currentFrame = (sync.currentFrame + 1) % sync.framesInFlight;
  } else {
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      // otherwise wait for submitted buffer to finish
      vkQueueWaitIdle(queue);
    } else {
      VK_CHECK_RESULT(result);
    }
  }

  return result;
}
