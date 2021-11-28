//
// Created by Бушев Дмитрий on 28.10.2021.
//

#include "VulkanRenderImpl.h"
#include "GuiInterface.h"
#include "vulkan_impl/RenderJob.h"
#include "vulkan_impl/VulkanDescriptorManager.h"
#include "vulkan_impl/VulkanDevice.h"
#include "vulkan_impl/VulkanMemoryManager.h"
#include "vulkan_impl/VulkanPipeline.h"
#include "vulkan_impl/VulkanQueueManager.h"
#include "vulkan_impl/VulkanSampler.h"
#include "vulkan_impl/VulkanShaderManager.h"
#include "vulkan_impl/VulkanSwapChain.h"
#include "vulkan_impl/util/VulkanDebug.h"
#include "vulkan_impl/util/VulkanInitializers.h"
#include "vulkan_impl/util/macro.h"
#include <iostream>

APITest::VulkanRenderImpl::VulkanRenderImpl(APITest::WindowRef const &window,
                                            bool enableValidation)
    : window_(window), validationLayersEnable(enableValidation),
      windowInterface(dynamic_cast<WindowInterface *>(window.get())) {

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "APITest";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "APITest";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  std::vector<const char *> reqExtensions{};
  if (window_)
    reqExtensions = windowInterface->getVulkanExtensions();

  if (validationLayersEnable) {
    const char *validationLayerName = "VK_LAYER_KHRONOS_validation";
    uint32_t instanceLayerCount;
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
    std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
    vkEnumerateInstanceLayerProperties(&instanceLayerCount,
                                       instanceLayerProperties.data());
    bool validationLayerPresent = false;
    for (VkLayerProperties layer : instanceLayerProperties) {
      if (strcmp(layer.layerName, validationLayerName) == 0) {
        validationLayerPresent = true;
        break;
      }
    }
    if (validationLayerPresent) {
      createInfo.ppEnabledLayerNames = &validationLayerName;
      createInfo.enabledLayerCount = 1;
    } else {
      createInfo.enabledLayerCount = 0;
      validationLayersEnable = false;
      std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, "
                   "validation is disabled"
                << std::endl;
    }
  }

  // validation layer requires VK_EXT_debug_utils extension
  if (validationLayersEnable)
    reqExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  createInfo.enabledExtensionCount = reqExtensions.size();
  createInfo.ppEnabledExtensionNames = reqExtensions.data();

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    throw std::runtime_error("call to vkCreateInstance() failed");

  if (validationLayersEnable) {
    // The report flags determine what type of messages for the layers will be
    // displayed For validating (debugging) an application the error and warning
    // bits should suffice
    VkDebugReportFlagsEXT debugReportFlags =
        VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    // Additional flags include performance info, loader and layer debug
    // messages, etc.
    debug::setupDebugging(instance, debugReportFlags, VK_NULL_HANDLE);

    std::cout << "Vulkan Validation layers enabled" << std::endl;
  }

  // Create vulkan device

  device = new VulkanDevice(this, window_.operator bool());

  // Create command queue manager

  queueManager = new VulkanQueueManager(device);

  // create swap chain only if window has been passed

  if (window_) {
    swapChain =
        new VulkanSwapChain(this, windowInterface->getSurface(instance));
    swapChain->recreate(&windowInterface->width_, &windowInterface->height_,
                        false);

    assignCallbacks();

    createCommandBuffers();
  }

  shaderManager = new VulkanShaderManager(this);

  pipelineManager = new VulkanPipelineManager(this);

  memoryManager = new VulkanMemoryManager(this);

  descriptorManager = new VulkanDescriptorManager(this);
}

APITest::VulkanRenderImpl::~VulkanRenderImpl() {

  device->waitIdle();

  debug::freeDebugCallback(instance);

  gui_.reset();

  delete descriptorManager;

  delete renderJob;

  delete memoryManager;

  delete pipelineManager;

  delete shaderManager;

  delete swapChain;

  delete queueManager;

  delete device;

  vkDestroyInstance(instance, nullptr);
}

void APITest::VulkanRenderImpl::connectWindow(APITest::WindowRef &&window) {
  // TODO

  assert(0 && "dynamic window connection is not supported yet");
}

APITest::ImageRef APITest::VulkanRenderImpl::createImage(ImageDesc desc) {
  // TODO
  return std::shared_ptr<Image>(new VulkanGeneralImage(memoryManager, desc));
}

bool APITest::VulkanRenderImpl::render() {

  statRecorder.endFrame();
  statRecorder.beginFrame("Frame start");

  if (swapChain && renderJob) {

    if (window_->minimized())
      return false;

    if (gui_) {
      statRecorder.push("GUI");
      statRecorder.stamp("update");
      guiInterface->update();
      statRecorder.stamp("render");
      guiInterface->render();
      statRecorder.pop();
    }
    statRecorder.stamp("SwapChain->acquireNextImage()");

    uint32_t imageIndex = 0;
    auto result = swapChain->acquireNextImage(&imageIndex);
    if (result != VK_SUCCESS) {
      windowResized();
      VK_CHECK_RESULT(swapChain->acquireNextImage(&imageIndex));
    }

    recordCommandBuffers(imageIndex);

    statRecorder.stamp("SwapChain->Submit()");

    if (swapChain->submitAndPresent(commandBuffers.data() + imageIndex) !=
        VK_SUCCESS) {
      windowResized();
    }
  }

  statRecorder.stamp("User Code");
  return false;
}

void APITest::VulkanRenderImpl::createCommandBuffers() {
  commandBuffers.resize(swapChain->imageCount);

  queueManager->createPrimaryCommandBuffers(commandBuffers.data(),
                                            commandBuffers.size());

  VkCommandBufferBeginInfo cmdBufInfo = initializers::commandBufferBeginInfo();

  // cmdBufInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  for (auto &buffer : commandBuffers) {
    VK_CHECK_RESULT(vkBeginCommandBuffer(buffer, &cmdBufInfo))

    // empty here for now ;))

    VK_CHECK_RESULT(vkEndCommandBuffer(buffer))
  }
}

static VkShaderStageFlagBits convertVkStage(APITest::ShaderStage stage) {
  switch (stage) {
  case APITest::ShaderStage::VERTEX:
    return VK_SHADER_STAGE_VERTEX_BIT;
  case APITest::ShaderStage::FRAGMENT:
    return VK_SHADER_STAGE_FRAGMENT_BIT;
  default:
    assert(0 && "Invalid shader stage enum value");
  }
  return VK_SHADER_STAGE_ALL;
}

APITest::ShaderRef APITest::VulkanRenderImpl::createShaderProgram(
    const APITest::ShaderDesc &shaderDesc) {
  return shaderManager->loadShader(shaderDesc.filename,
                                   convertVkStage(shaderDesc.stage),
                                   shaderDesc.filename);
}

void APITest::VulkanRenderImpl::recordCommandBuffers(int swapChainImage) {

  VkCommandBufferBeginInfo cmdBufInfo = initializers::commandBufferBeginInfo();

  auto &buffer = commandBuffers.at(swapChainImage);
  VK_CHECK_RESULT(vkBeginCommandBuffer(buffer, &cmdBufInfo))

  renderJob->compile(swapChainImage, buffer);

  VK_CHECK_RESULT(vkEndCommandBuffer(buffer))
}

void APITest::VulkanRenderImpl::windowResized() {
  swapChain->recreate(&windowInterface->width_, &windowInterface->height_,
                      false);
  if (renderJob)
    renderJob->resetFrameBuffers();
}

APITest::GraphicsPipelineRef APITest::VulkanRenderImpl::createGraphicsPipeline(
    APITest::GraphicsPipelineLayout layout) {
  return pipelineManager->get(layout);
}

void APITest::VulkanRenderImpl::waitIdle() { device->waitIdle(); }

APITest::VertexBufferRef APITest::VulkanRenderImpl::createVertexBuffer(
    void *initialData, size_t initialSize, APITest::MemoryType memType) {
  auto ret = std::unique_ptr<VertexBuffer>(
      new VulkanVertexBuffer(memoryManager, memType, initialSize));

  if (initialData != nullptr)
    ret->push(initialData, initialSize, 0);

  return std::move(ret);
}

APITest::IndexBufferRef APITest::VulkanRenderImpl::createIndexBuffer(
    void *initialData, size_t initialSize, APITest::IndexBuffer::Type indexType,
    APITest::MemoryType memType) {
  auto ret = std::unique_ptr<IndexBuffer>(
      new VulkanIndexBuffer(memoryManager, memType, initialSize, indexType));

  if (initialData != nullptr)
    ret->push(initialData, initialSize, 0);

  return std::move(ret);
}

void APITest::VulkanRenderImpl::pushNewRenderPassGraph(
    APITest::RenderPassRef node) {
  delete renderJob;

  renderJob = new RenderJob(this, node);
}

APITest::OnscreenRenderPassRef
APITest::VulkanRenderImpl::createOnscreenColorPass() {
  return OnscreenRenderPassRef(new VulkanOnscreenRenderPass(this));
}

APITest::GUIRef APITest::VulkanRenderImpl::getGUI() {
  if (!gui_) {
    gui_ = createImGui(this);
    guiInterface = dynamic_cast<GuiInterface *>(gui_.get());
  }

  return gui_;
}

void APITest::VulkanRenderImpl::assignCallbacks() {
  if (!windowInterface)
    return;

  windowInterface->callback.keyDown = [this](int key) {
    if (gui_)
      guiInterface->keyDown(key);
  };
  windowInterface->callback.keyUp = [this](int key) {
    if (gui_)
      guiInterface->keyUp(key);
  };

  windowInterface->callback.keyPressed = [this](int key) {
    if (gui_)
      guiInterface->mouseBtnUp(key);
  };

  windowInterface->callback.charInput = [this](int key) {
    if (gui_)
      guiInterface->charInput(key);
  };

  windowInterface->callback.mouseScroll = [this](double x, double y) {
    if (gui_)
      guiInterface->mouseScroll(x, y);
  };

  windowInterface->callback.mouseMoved = [this](double x, double y) {
    if (gui_)
      guiInterface->mouseMoved(x, y);
  };

  windowInterface->callback.mouseBtnDown = [this](int btn) {
    if (gui_)
      guiInterface->mouseBtnDown(btn);
  };

  windowInterface->callback.mouseBtnUp = [this](int btn) {
    if (gui_)
      guiInterface->mouseBtnUp(btn);
  };
}

APITest::WindowRef APITest::VulkanRenderImpl::getWindow() const {
  return window_;
}

APITest::DescriptorSetLayoutRef
APITest::VulkanRenderImpl::createDescriptorLayout(
    const std::vector<DescriptorLayout> &desc) {
  return descriptorManager->createNewDescriptorSetLayout(desc);
}

APITest::SamplerRef
APITest::VulkanRenderImpl::createSampler(const APITest::SamplerDesc &desc) {
  return SamplerRef(new VulkanSampler(this, desc));
}

APITest::UniformBufferRef APITest::VulkanRenderImpl::createUniformBuffer(
    void *initialData, size_t initialSize, APITest::MemoryType memType) {
  auto buffer = APITest::UniformBufferRef(
      new VulkanUniformBuffer(memoryManager, memType, initialSize));
  if (initialData)
    buffer->push(initialData, initialSize, 0);
  return std::move(buffer);
}
