//
// Created by Бушев Дмитрий on 28.10.2021.
//

#ifndef RENDERAPITEST_VULKANRENDERIMPL_H
#define RENDERAPITEST_VULKANRENDERIMPL_H

#include "RenderInterface.h"
#include "WindowInterface.h"
#include "common.h"
#include "vulkan_impl/util/UniversalContainer.h"

namespace APITest {

class VulkanDevice;
class VulkanQueueManager;
class VulkanSwapChain;
class VulkanShaderManager;
class VulkanPipelineManager;
class VulkanMemoryManager;
class VulkanCommand;
class RenderJob;
class GuiInterface;
class VulkanDescriptorManager;

class VulkanRenderImpl final : public RenderInterface {
  WindowRef window_ = nullptr;
  WindowInterface *windowInterface = nullptr;
  GUIRef gui_ = nullptr;
  GuiInterface *guiInterface = nullptr;
  bool validationLayersEnable = false;
  VkInstance instance = VK_NULL_HANDLE;
  ImageRef swapChainRef = 0;

  VulkanDevice *device = nullptr;
  VulkanQueueManager *queueManager = nullptr;
  VulkanSwapChain *swapChain = nullptr;
  VulkanShaderManager *shaderManager = nullptr;
  VulkanPipelineManager *pipelineManager = nullptr;
  VulkanMemoryManager *memoryManager = nullptr;
  VulkanDescriptorManager *descriptorManager = nullptr;

  RenderJob *renderJob = nullptr;
  std::vector<VkCommandBuffer> commandBuffers;

  StatisticsRecorder statRecorder;

  void createCommandBuffers();
  void recordCommandBuffers(int swapChainImage);
  void windowResized();
  void assignCallbacks();

public:
  explicit VulkanRenderImpl(WindowRef const &window = nullptr,
                            bool enableValidation = true);

  VkInstance getInstance() const { return instance; }
  const VulkanQueueManager *getQueueManager() const { return queueManager; }
  const VulkanDevice *getVulkanDevice() const { return device; }
  const VulkanShaderManager *getShaderManager() const { return shaderManager; }
  const VulkanSwapChain *getSwapChain() const { return swapChain; }
  VulkanPipelineManager *getPipelineManager() const { return pipelineManager; }
  VulkanMemoryManager *getMemoryManager() const { return memoryManager; }
  GuiInterface *getGuiInterface() const { return guiInterface; }
  bool isSwapChain(ImageRef image) const { return image == swapChainRef; }

  void connectWindow(WindowRef &&window) override;

  ImageRef createImage(ImageDesc desc) override;

  SamplerRef createSampler(SamplerDesc const &desc) override;

  GraphicsPipelineRef
  createGraphicsPipeline(GraphicsPipelineLayout layout) override;

  VertexBufferRef
  createVertexBuffer(void *initialData, size_t initialSize,
                     MemoryType = MemoryType::HOST_VISIBLE) override;

  IndexBufferRef createIndexBuffer(
      void *initialData, size_t initialSize,
      IndexBuffer::Type indexType = IndexBuffer::Type::INDEX_TYPE_UINT_32,
      MemoryType = MemoryType::HOST_VISIBLE) override;

  /** @brief loads and compiles shader program */
  ShaderRef createShaderProgram(ShaderDesc const &shaderDesc) override;

  UniformBufferRef
  createUniformBuffer(void *initialData, size_t initialSize,
                      MemoryType = MemoryType::HOST_VISIBLE) override;

  /** @brief overrides previous render pass graph. */
  void pushNewRenderPassGraph(RenderPassRef node) override;

  OnscreenRenderPassRef createOnscreenColorPass() override;

  DescriptorSetLayoutRef
  createDescriptorLayout(std::vector<DescriptorLayout> const &desc) override;

  GUIRef getGUI() override;

  WindowRef getWindow() const override;

  bool render() override;

  void waitIdle() override;

  FrameStatistics const &statistics() const override {
    return statRecorder.stat();
  };
  StatisticsRecorder &recorder() { return statRecorder; };

  ~VulkanRenderImpl() override;
};

} // namespace APITest
#endif // RENDERAPITEST_VULKANRENDERIMPL_H
