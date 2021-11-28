//
// Created by Бушев Дмитрий on 01.11.2021.
//

#include "VulkanRenderPass.h"
#include "vulkan_impl/VulkanDevice.h"
#include "vulkan_impl/VulkanMemoryManager.h"
#include "vulkan_impl/VulkanRenderImpl.h"
#include "vulkan_impl/VulkanSwapChain.h"
#include "vulkan_impl/util/VulkanInitializers.h"
#include "vulkan_impl/util/macro.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <glm/glm.hpp>
#include <optional>

// legacy code, may reuse some of this later

#if 0
APITest::VulkanOnscreenRenderPass::VulkanOnscreenRenderPass(VulkanRenderImpl* parent, const std::vector<Attachment> &attachments):
    VulkanRenderPass(parent) {

    assert(std::count_if(attachments.begin(), attachments.end(),
                         [](Attachment const& elem){ return elem.type == Attachment::DEPTH;}) <= 1 &&
                            "There must be no more than one depth attachment");

    assert(std::all_of(attachments.begin(), attachments.end(), [&attachments](Attachment const& elem)
    { return elem.extents.first == attachments.front().extents.first &&
                elem.extents.second == attachments.front().extents.second;}) &&
                    "Image extents must match among all attachments");

    assert(std::all_of(attachments.begin(), attachments.end(), [&attachments](Attachment const& elem)
    { return elem.description.samples == 1;}) && "Multisampled attachments aren't supported yet");

    std::vector<VkAttachmentDescription> attachmentsDesc(attachments.size());

    std::transform(attachments.begin(), attachments.end(), attachmentsDesc.begin(), [](Attachment const& elem){
        return elem.description;
    });

    extents.width = attachments.front().extents.first;
    extents.height = attachments.front().extents.second;
    extents.depth = 1;

    std::optional<VkAttachmentReference> depthAttachmentRef;
    std::vector<VkAttachmentReference> colorAttachmentRefs{};

    auto depthAttachment = std::find_if(attachments.begin(), attachments.end(),
                                        [](Attachment const& elem){ return elem.type == Attachment::DEPTH;});

    if(depthAttachment != attachments.end()){
        depthAttachmentRef.emplace();
        depthAttachmentRef.value().attachment = depthAttachment - attachments.begin();
        depthAttachmentRef.value().layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    for(auto it = attachments.begin(), end = attachments.end(); it < end; ++it){
        if(it->type == Attachment::DEPTH)
            continue;

        colorAttachmentRefs.emplace_back();
        colorAttachmentRefs.back().layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.back().attachment = it - attachments.begin();
    }

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = colorAttachmentRefs.size();
    subpassDescription.pColorAttachments = colorAttachmentRefs.data();
    subpassDescription.pDepthStencilAttachment = depthAttachmentRef.has_value() ? &depthAttachmentRef.value() : nullptr;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    // Subpass dependencies for layout transitions
    std::array<VkSubpassDependency, 2> dependencies{};

    // TODO: deduct external dependencies by framebuffer images use info

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = attachmentsDesc.size();

    renderPassInfo.pAttachments = attachmentsDesc.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK_RESULT(
            vkCreateRenderPass(parent_->getVulkanDevice()->get(), &renderPassInfo, nullptr, &pass_))

    for(auto& attach: attachments){
        VkClearValue value;
        if(attach.type == VulkanOnscreenRenderPass::Attachment::COLOR) {
            value.color = {0.0f, 0.0f, 0.0f, 0.0f};
        } else {
            value.depthStencil = {1.0f, 0};
        }

        clearValues.push_back(value);
    }
}
#endif

APITest::VulkanOnscreenRenderPass::VulkanOnscreenRenderPass(
    VulkanRenderImpl *parent)
    : VulkanRenderPass(parent) {

  extents.width = parent->getSwapChain()->surfaceWidth;
  extents.height = parent->getSwapChain()->surfaceHeight;
  extents.depth = 1;
}
void APITest::VulkanOnscreenRenderPass::begin(VkCommandBuffer cmdBuffer,
                                              VkFramebuffer framebuffer) {
  VkRenderPassBeginInfo renderPassBeginInfo =
      initializers::renderPassBeginInfo();
  renderPassBeginInfo.renderPass = pass_;
  renderPassBeginInfo.renderArea.offset.x = 0;
  renderPassBeginInfo.renderArea.offset.y = 0;
  renderPassBeginInfo.renderArea.extent.width = extents.width;
  renderPassBeginInfo.renderArea.extent.height = extents.height;

  renderPassBeginInfo.clearValueCount = depthBuffer ? clearValues.size() : 1;
  renderPassBeginInfo.pClearValues = clearValues.data();
  renderPassBeginInfo.framebuffer = framebuffer;

  vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = initializers::viewport(
      (float)extents.width, (float)extents.height, 0.0f, 1.0f);
  vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

  VkRect2D scissor = initializers::rect2D(extents.width, extents.height, 0, 0);
  vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
}

void APITest::VulkanOnscreenRenderPass::end(VkCommandBuffer cmdBuffer) {
  vkCmdEndRenderPass(cmdBuffer);
}

VkFramebuffer APITest::VulkanOnscreenRenderPass::allocateFrameBuffer(
    VkImageView swapChainImage) {

  std::vector<VkImageView> images;
  images.push_back(swapChainImage);
  if (depthBuffer)
    images.push_back(depthBuffer->getDepthView());

  VkFramebufferCreateInfo frameBufferCreateInfo = {};
  frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  frameBufferCreateInfo.pNext = nullptr;
  frameBufferCreateInfo.renderPass = pass_;
  frameBufferCreateInfo.width = extents.width;
  frameBufferCreateInfo.height = extents.height;
  frameBufferCreateInfo.layers = 1;
  frameBufferCreateInfo.pAttachments = images.data();
  frameBufferCreateInfo.attachmentCount = images.size();

  VkFramebuffer ret;
  VK_CHECK_RESULT(vkCreateFramebuffer(parent_->getVulkanDevice()->get(),
                                      &frameBufferCreateInfo, nullptr, &ret))
  return ret;
}

void APITest::VulkanOnscreenRenderPass::changeExtents(int newWidth,
                                                      int newHeight) {
  extents.width = newWidth;
  extents.height = newHeight;

  if (depthBuffer) {
    // depth buffer extents should match, so we need to reallocate image to
    // trunc it to new size
    delete depthBuffer;
    depthBuffer = new VulkanDepthBuffer(
        parent_->getMemoryManager(),
        {parent_->getVulkanDevice()->getSupportedDepthFormat(false),
         {extents.width, extents.height},
         0});
  }
}

void APITest::VulkanOnscreenRenderPass::enableDepthBuffer(uint32_t binding) {
  if (!depthBuffer) {
    depthBuffer = new VulkanDepthBuffer(
        parent_->getMemoryManager(),
        {parent_->getVulkanDevice()->getSupportedDepthFormat(false),
         {extents.width, extents.height},
         0});
  }

  bindings.second = binding;
}

APITest::VulkanOnscreenRenderPass::~VulkanOnscreenRenderPass() {
  delete depthBuffer;
}

static VkAttachmentLoadOp
convertToVulkanLoadOp(APITest::OffscreenRenderPass::LoadOp loadop) {
  switch (loadop) {
  case APITest::OffscreenRenderPass::LoadOp::LOAD:
    return VK_ATTACHMENT_LOAD_OP_LOAD;
  case APITest::OffscreenRenderPass::LoadOp::DONT_CARE:
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  case APITest::OffscreenRenderPass::LoadOp::CLEAR:
    return VK_ATTACHMENT_LOAD_OP_CLEAR;
  }

  return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

static VkAttachmentStoreOp
convertToVulkanStoreOp(APITest::OffscreenRenderPass::StoreOp storeOp) {
  switch (storeOp) {
  case APITest::OffscreenRenderPass::StoreOp::STORE:
    return VK_ATTACHMENT_STORE_OP_STORE;
  case APITest::OffscreenRenderPass::StoreOp::DONT_CARE:
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
  }

  return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

void APITest::VulkanOnscreenRenderPass::compile() {

  std::vector<VkAttachmentDescription> attachmentsDesc{depthBuffer ? 2u : 1u};

  attachmentsDesc[0].format = parent_->getSwapChain()->colorFormat;
  attachmentsDesc[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentsDesc[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentsDesc[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  attachmentsDesc[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachmentsDesc[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentsDesc[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentsDesc[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  if (depthBuffer) {
    attachmentsDesc[1].format = depthBuffer->format();
    attachmentsDesc[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentsDesc[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentsDesc[1].finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachmentsDesc[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentsDesc[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentsDesc[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentsDesc[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  }

  std::optional<VkAttachmentReference> depthAttachmentRef;
  VkAttachmentReference colorAttachmentRef;

  if (depthBuffer) {
    depthAttachmentRef.emplace();
    depthAttachmentRef.value().layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachmentRef.value().attachment = depthBufferAttachInfo.binding;
  }

  colorAttachmentRef.attachment = colorAttachInfo.binding;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments = &colorAttachmentRef;
  subpassDescription.pDepthStencilAttachment =
      depthAttachmentRef.has_value() ? &depthAttachmentRef.value() : nullptr;
  subpassDescription.inputAttachmentCount = 0;
  subpassDescription.pInputAttachments = nullptr;
  subpassDescription.preserveAttachmentCount = 0;
  subpassDescription.pPreserveAttachments = nullptr;
  subpassDescription.pResolveAttachments = nullptr;

  // Subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies{};

  // TODO: deduct external dependencies by framebuffer images use info

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = attachmentsDesc.size();

  renderPassInfo.pAttachments = attachmentsDesc.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  VK_CHECK_RESULT(vkCreateRenderPass(parent_->getVulkanDevice()->get(),
                                     &renderPassInfo, nullptr, &pass_))

  clearValues.push_back({0.0f, 0.0f, 0.0f, 0.0f});
  clearValues.push_back({1.0, 0.0f});
}

void APITest::VulkanOnscreenRenderPass::setColorBufferIndex(uint32_t binding) {
  colorAttachInfo.binding = binding;
}

APITest::VulkanRenderPass::~VulkanRenderPass() {
  if (pass_ != VK_NULL_HANDLE)
    vkDestroyRenderPass(parent_->getVulkanDevice()->get(), pass_, nullptr);
}
