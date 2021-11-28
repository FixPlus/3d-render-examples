//
// Created by Бушев Дмитрий on 28.10.2021.
//

#ifndef RENDERAPITEST_VULKANDEVICE_H
#define RENDERAPITEST_VULKANDEVICE_H

#include "vulkan/vulkan.h"
#include <string>
#include <vector>

namespace APITest {

class VulkanRenderImpl;

class VulkanDevice final {
  /** @brief Physical device representation */
  VkPhysicalDevice physicalDevice_;
  /** @brief Logical device representation (application's view of the device) */
  VkDevice logicalDevice_;
  /** @brief Set to true when the debug marker extension is detected */
  bool enableDebugMarkers = false;

  const VulkanRenderImpl *parent_;

  void queryPhysicalDeviceInfo();
  bool createLogicalDevice(bool useSwapChain);
  uint32_t getQueueFamilyIndex(VkQueueFlagBits queueFlags) const;

public:
  /** @brief Properties of the physical device including limits that the
   * application can check against */
  VkPhysicalDeviceProperties properties{};
  /** @brief Features of the physical device that an application can use to
   * check if a feature is supported */
  VkPhysicalDeviceFeatures features{};
  /** @brief Features that have been enabled for use on the physical device */
  VkPhysicalDeviceFeatures enabledFeatures{};
  /** @brief Memory types and heaps of the physical device */
  VkPhysicalDeviceMemoryProperties memoryProperties{};
  /** @brief Queue family properties of the physical device */
  std::vector<VkQueueFamilyProperties> queueFamilyProperties{};
  /** @brief List of extensions supported by the device */
  std::vector<std::string> supportedExtensions{};

  struct {
    uint32_t graphics;
    uint32_t compute;
    uint32_t transfer;
  } queueFamilyIndices;

  explicit VulkanDevice(const VulkanRenderImpl *parent,
                        bool useSwapChain = false);

  bool extensionSupported(std::string const &extension);
  VkFormat getSupportedDepthFormat(bool checkSamplingSupport) const;

  ~VulkanDevice();

  VkDevice get() const { return logicalDevice_; }
  VkPhysicalDevice getPhysical() const { return physicalDevice_; }

  void waitIdle();
};
} // namespace APITest
#endif // RENDERAPITEST_VULKANDEVICE_H
