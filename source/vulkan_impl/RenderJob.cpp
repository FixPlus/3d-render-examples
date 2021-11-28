//
// Created by Бушев Дмитрий on 07.11.2021.
//

#include "RenderJob.h"
#include "GuiInterface.h"
#include "vulkan_impl/VulkanCommandRecorder.h"
#include "vulkan_impl/VulkanDevice.h"
#include "vulkan_impl/VulkanPipeline.h"
#include "vulkan_impl/VulkanRenderImpl.h"
#include "vulkan_impl/VulkanSwapChain.h"
#include <cassert>

APITest::RenderJob::RenderJob(APITest::VulkanRenderImpl *parent,
                              RenderPassRef desc)
    : parent_(parent) {
  compileRenderPass(desc);
}

void APITest::RenderJob::compileRenderPass(RenderPassRef desc) {
  auto pass = desc.lock();
  for (auto const &dep : pass->dependencies)
    compileRenderPass(dep);

  auto *vkPass = dynamic_cast<VulkanRenderPass *>(pass.get());

  vkPass->compile();

  auto &unit = renderSequence.emplace_back(desc);

  if (auto *onScreenPass = dynamic_cast<VulkanOnscreenRenderPass *>(vkPass)) {
    auto *swapChain = parent_->getSwapChain();
    int imageCount = swapChain->buffers.size();
    for (int i = 0; i < imageCount; i++) {
      unit.frameBuffers.push_back(
          onScreenPass->allocateFrameBuffer(swapChain->buffers.at(i).view));
    }
  } else {
    throw std::runtime_error("[Vulkan][ERROR] Render Pass graph compilation "
                             "failed: offscreen passes aren't supported yet.");
  }
}

void APITest::RenderJob::resetFrameBuffers() {
  for (auto &unit : renderSequence) {
    auto pass = unit.pass.lock();
    if (auto *onScreenPass =
            dynamic_cast<VulkanOnscreenRenderPass *>(pass.get())) {
      for (auto buffer : unit.frameBuffers)
        vkDestroyFramebuffer(parent_->getVulkanDevice()->get(), buffer,
                             nullptr);
      unit.frameBuffers.clear();
      auto *swapChain = parent_->getSwapChain();
      int imageCount = swapChain->buffers.size();
      onScreenPass->changeExtents(swapChain->surfaceWidth,
                                  swapChain->surfaceHeight);
      for (int i = 0; i < imageCount; i++) {
        unit.frameBuffers.push_back(
            onScreenPass->allocateFrameBuffer(swapChain->buffers.at(i).view));
      }
    }
  }
}

void APITest::RenderJob::compile(int swapChainImage,
                                 VkCommandBuffer commandBuffer) {

  auto &statRecorder = parent_->recorder();

  statRecorder.push("Command record");

  VulkanCommandRecorder commandRecorder;

  commandRecorder.bufferToRecord = commandBuffer;

  for (auto &unit : renderSequence) {
    auto pass = unit.pass.lock();
    statRecorder.stamp(
        "Render Pass " +
        std::to_string(reinterpret_cast<unsigned long long>(pass.get())));
    auto *vkPass = dynamic_cast<VulkanRenderPass *>(pass.get());
    auto currentBuffer = unit.frameBuffers.size() == 1
                             ? unit.frameBuffers.front()
                             : unit.frameBuffers.at(swapChainImage);
    vkPass->begin(commandBuffer, currentBuffer);

    commandRecorder.currentPass = vkPass;

    vkPass->commands(&commandRecorder);

    if (dynamic_cast<OnscreenRenderPass *>(vkPass)) {
      auto *gui = parent_->getGuiInterface();
      if (gui)
        gui->draw(&commandRecorder);
    }

    vkPass->end(commandBuffer);
  }

  statRecorder.pop();
}

APITest::RenderJob::~RenderJob() {
  for (auto &unit : renderSequence)
    for (auto buffer : unit.frameBuffers)
      vkDestroyFramebuffer(parent_->getVulkanDevice()->get(), buffer, nullptr);
}
