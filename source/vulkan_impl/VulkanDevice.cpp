//
// Created by Бушев Дмитрий on 28.10.2021.
//

#include "VulkanDevice.h"
#include "VulkanRenderImpl.h"
#include <cassert>
#include <iostream>

APITest::VulkanDevice::VulkanDevice(const APITest::VulkanRenderImpl *parent,
                                    bool useSwapChain)
    : parent_(parent) {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(parent_->getInstance(), &deviceCount, nullptr);
  if (deviceCount == 0)
    throw std::runtime_error("There are no devices supporting Vulkan.");

  std::vector<VkPhysicalDevice> availableDevices(deviceCount);
  vkEnumeratePhysicalDevices(parent_->getInstance(), &deviceCount,
                             availableDevices.data());

  physicalDevice_ = availableDevices[0];

  queryPhysicalDeviceInfo();

  for (int i = 0; i < availableDevices.size(); i++) {
    try {
      createLogicalDevice(useSwapChain);
    } catch (std::runtime_error &e) {
      if (i == availableDevices.size() - 1)
        throw e;

      continue;
    }
  }
}

void APITest::VulkanDevice::queryPhysicalDeviceInfo() {
  // Store Properties features, limits and properties of the physical device for
  // later use Device properties also contain limits and sparse properties
  vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
  // Features should be checked by the examples before using them
  vkGetPhysicalDeviceFeatures(physicalDevice_, &features);
  // Memory properties are used regularly for creating all kinds of buffers
  vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);
  // Queue family properties, used for setting up requested queues upon device
  // creation
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount,
                                           nullptr);
  assert(queueFamilyCount > 0);
  queueFamilyProperties.resize(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount,
                                           queueFamilyProperties.data());

  // Get list of supported extensions
  uint32_t extCount = 0;
  vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr, &extCount,
                                       nullptr);
  if (extCount > 0) {
    std::vector<VkExtensionProperties> extensions(extCount);
    if (vkEnumerateDeviceExtensionProperties(physicalDevice_, nullptr,
                                             &extCount, &extensions.front()) ==
        VK_SUCCESS) {
      for (auto ext : extensions) {
        supportedExtensions.emplace_back(ext.extensionName);
      }
    }
  }

  std::cout << "Vulkan picked following GPU for a test: "
            << properties.deviceName << std::endl;
}

bool APITest::VulkanDevice::createLogicalDevice(bool useSwapChain) {

  auto requestedQueueTypes = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT;
  // Desired queues need to be requested upon logical device creation
  // Due to differing queue family configurations of Vulkan implementations this
  // can be a bit tricky, especially if the application requests different queue
  // types

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

  // Get queue family indices for the requested queue family types
  // Note that the indices may overlap depending on the implementation

  const float defaultQueuePriority(0.0f);

  // Graphics queue
  if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT) {
    queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &defaultQueuePriority;
    queueCreateInfos.push_back(queueInfo);
  } else {
    queueFamilyIndices.graphics = 0;
  }

  // Dedicated compute queue
  if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT) {
    queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
    if (queueFamilyIndices.compute != queueFamilyIndices.graphics) {
      // If compute family index differs, we need an additional queue create
      // info for the compute queue
      VkDeviceQueueCreateInfo queueInfo{};
      queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
      queueInfo.queueCount = 1;
      queueInfo.pQueuePriorities = &defaultQueuePriority;
      queueCreateInfos.push_back(queueInfo);
    }
  } else {
    // Else we use the same queue
    queueFamilyIndices.compute = queueFamilyIndices.graphics;
  }

  // Dedicated transfer queue
  if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT) {
    queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
    if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) &&
        (queueFamilyIndices.transfer != queueFamilyIndices.compute)) {
      // If compute family index differs, we need an additional queue create
      // info for the compute queue
      VkDeviceQueueCreateInfo queueInfo{};
      queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
      queueInfo.queueCount = 1;
      queueInfo.pQueuePriorities = &defaultQueuePriority;
      queueCreateInfos.push_back(queueInfo);
    }
  } else {
    // Else we use the same queue
    queueFamilyIndices.transfer = queueFamilyIndices.graphics;
  }

  // Create the logical device representation
  std::vector<const char *> deviceExtensions{};
  if (useSwapChain) {
    // If the device will be used for presenting to a display via a swapchain we
    // need to request the swapchain extension
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  }

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

  // Enable the debug marker extension if it is present (likely meaning a
  // debugging tool is present)
  if (extensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME)) {
    deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
    enableDebugMarkers = true;
  }

  if (!deviceExtensions.empty()) {
    for (const char *enabledExtension : deviceExtensions) {
      if (!extensionSupported(enabledExtension)) {
        throw std::runtime_error(
            "[VULKAN][ERROR] device has no support for enabled extension " +
            std::string(enabledExtension));
      }
    }

    deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
  }

  VkResult result = vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr,
                                   &logicalDevice_);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("[VULKAN][ERROR] failed to create device");
  }

  return result;
}

uint32_t
APITest::VulkanDevice::getQueueFamilyIndex(VkQueueFlagBits queueFlags) const {
  // Dedicated queue for compute
  // Try to find a queue family index that supports compute but not graphics
  if (queueFlags & VK_QUEUE_COMPUTE_BIT) {
    for (uint32_t i = 0;
         i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
      if ((queueFamilyProperties[i].queueFlags & queueFlags) &&
          ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ==
           0)) {
        return i;
      }
    }
  }

  // Dedicated queue for transfer
  // Try to find a queue family index that supports transfer but not graphics
  // and compute
  if (queueFlags & VK_QUEUE_TRANSFER_BIT) {
    for (uint32_t i = 0;
         i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
      if ((queueFamilyProperties[i].queueFlags & queueFlags) &&
          ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) ==
           0) &&
          ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)) {
        return i;
      }
    }
  }

  // For other queue types or if no separate compute queue is present, return
  // the first one to support the requested flags
  for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size());
       i++) {
    if (queueFamilyProperties[i].queueFlags & queueFlags) {
      return i;
    }
  }

  throw std::runtime_error("Could not find a matching queue family index");
}

bool APITest::VulkanDevice::extensionSupported(std::string const &extension) {
  return (std::find(supportedExtensions.begin(), supportedExtensions.end(),
                    extension) != supportedExtensions.end());
}

VkFormat APITest::VulkanDevice::getSupportedDepthFormat(
    bool checkSamplingSupport) const {
  // All depth formats may be optional, so we need to find a suitable depth
  // format to use
  std::vector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
      VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT,
      VK_FORMAT_D16_UNORM};
  for (auto &format : depthFormats) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice_, format,
                                        &formatProperties);
    // Format must support depth stencil attachment for optimal tiling
    if (formatProperties.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      if (checkSamplingSupport) {
        if (!(formatProperties.optimalTilingFeatures &
              VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
          continue;
        }
      }
      return format;
    }
  }
  throw std::runtime_error("Could not find a matching depth format");
}

APITest::VulkanDevice::~VulkanDevice() {
  if (logicalDevice_) {
    vkDestroyDevice(logicalDevice_, nullptr);
  }
}

void APITest::VulkanDevice::waitIdle() { vkDeviceWaitIdle(logicalDevice_); }
