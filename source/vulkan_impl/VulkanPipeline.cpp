//
// Created by Бушев Дмитрий on 06.11.2021.
//

#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "VulkanRenderImpl.h"
#include "vulkan_impl/VulkanDescriptorManager.h"
#include "vulkan_impl/VulkanRenderPass.h"
#include "vulkan_impl/VulkanShaderManager.h"
#include "vulkan_impl/util/VulkanInitializers.h"
#include "vulkan_impl/util/macro.h"
#include <array>
#include <glm/glm.hpp>

APITest::VulkanPipelineManager::VulkanPipelineManager(
    APITest::VulkanRenderImpl *parent)
    : parent_(parent) {
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VK_CHECK_RESULT(vkCreatePipelineCache(parent->getVulkanDevice()->get(),
                                        &pipelineCacheCreateInfo, nullptr,
                                        &pipelineCache));
}

APITest::VulkanPipelineManager::~VulkanPipelineManager() {
  auto device = parent_->getVulkanDevice()->get();

  vkDestroyPipelineCache(device, pipelineCache, nullptr);
}

static std::pair<uint32_t, std::vector<VkVertexInputAttributeDescription>>
vertexStateCI(APITest::GraphicsPipelineLayout const &desc) {
  uint32_t perVertexSize = 0;

  std::vector<VkVertexInputAttributeDescription> attributesDesc;

  auto &format = desc.vertexLayout.perVertexAttribute;

  for (auto attr : format) {
    switch (attr) {
    case APITest::VertexLayout::R32SF:
      attributesDesc.push_back(VkVertexInputAttributeDescription(
          {static_cast<uint32_t>(attributesDesc.size()), 0,
           VK_FORMAT_R32_SFLOAT, perVertexSize}));
      perVertexSize += sizeof(float);
      break;
    case APITest::VertexLayout::RG32SF:
      attributesDesc.push_back(VkVertexInputAttributeDescription(
          {static_cast<uint32_t>(attributesDesc.size()), 0,
           VK_FORMAT_R32G32_SFLOAT, perVertexSize}));
      perVertexSize += sizeof(glm::vec2);
      break;
    case APITest::VertexLayout::RGB32SF:
      attributesDesc.push_back(VkVertexInputAttributeDescription(
          {static_cast<uint32_t>(attributesDesc.size()), 0,
           VK_FORMAT_R32G32B32_SFLOAT, perVertexSize}));
      perVertexSize += sizeof(glm::vec3);
      break;
    case APITest::VertexLayout::RGBA32SF:
      attributesDesc.push_back(VkVertexInputAttributeDescription(
          {static_cast<uint32_t>(attributesDesc.size()), 0,
           VK_FORMAT_R32G32B32A32_SFLOAT, perVertexSize}));
      perVertexSize += sizeof(glm::vec4);
      break;
    case APITest::VertexLayout::RGBA8UNORM:
      attributesDesc.push_back(VkVertexInputAttributeDescription(
          {static_cast<uint32_t>(attributesDesc.size()), 0,
           VK_FORMAT_R8G8B8A8_UNORM, perVertexSize}));
      perVertexSize += 4;
      break;
    case APITest::VertexLayout::MAT4F:
      for (int i = 0; i < 4; ++i) {
        attributesDesc.push_back(VkVertexInputAttributeDescription(
            {static_cast<uint32_t>(attributesDesc.size()), 0,
             VK_FORMAT_R32G32B32A32_SFLOAT, perVertexSize}));
        perVertexSize += sizeof(glm::vec4);
      }
      break;
    }
  }

  return {perVertexSize, attributesDesc};
}

APITest::GraphicsPipelineRef APITest::VulkanPipelineManager::get(
    APITest::GraphicsPipelineLayout const &desc) {
  return std::unique_ptr<GraphicsPipeline>(
      new VulkanGraphicsPipeline(this, desc));
}

APITest::VulkanGraphicsPipeline::VulkanGraphicsPipeline(
    APITest::VulkanPipelineManager *parent,
    APITest::GraphicsPipelineLayout layout)
    : VulkanPipeline(parent), layout_(std::move(layout)) {

  VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
  if (auto *vkSetLayout = dynamic_cast<VulkanDescriptorSetLayout *>(
          layout_.descriptorsLayout.get())) {
    setLayout = vkSetLayout->layout();
  }
  VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
      initializers::pipelineLayoutCreateInfo(
          &setLayout, setLayout != VK_NULL_HANDLE ? 1 : 0);

  VK_CHECK_RESULT(vkCreatePipelineLayout(
      parent_->parent_->getVulkanDevice()->get(), &pPipelineLayoutCreateInfo,
      nullptr, &layoutHandle));
}

static VkCompareOp
convertToVkCompareOp(APITest::RasterizerLayout::DepthTest::CompOp compOp) {
  switch (compOp) {
  case APITest::RasterizerLayout::DepthTest::CompOp::LESS_OR_EQUAL:
    return VK_COMPARE_OP_LESS_OR_EQUAL;
  case APITest::RasterizerLayout::DepthTest::CompOp::LESS:
    return VK_COMPARE_OP_LESS;
  case APITest::RasterizerLayout::DepthTest::CompOp::GREATER:
    return VK_COMPARE_OP_GREATER;
  case APITest::RasterizerLayout::DepthTest::CompOp::GREATER_OR_EQUAL:
    return VK_COMPARE_OP_GREATER_OR_EQUAL;
  default:
    return VK_COMPARE_OP_MAX_ENUM;
  }
}

static VkCullModeFlags
convertCullMode(APITest::RasterizerLayout::CullingState::CullMode mode) {
  switch (mode) {
  case APITest::RasterizerLayout::CullingState::CULL_NONE:
    return VK_CULL_MODE_NONE;
  case APITest::RasterizerLayout::CullingState::CULL_FRONT_FACE:
    return VK_CULL_MODE_FRONT_BIT;
  case APITest::RasterizerLayout::CullingState::CULL_BACK_FACE:
    return VK_CULL_MODE_BACK_BIT;
  }
  return VK_CULL_MODE_FRONT_AND_BACK;
}

static VkFrontFace
convertFrontFace(APITest::RasterizerLayout::CullingState::FrontFace face) {
  if (face == APITest::RasterizerLayout::CullingState::FRONT_FACE_CCW)
    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
  else
    return VK_FRONT_FACE_CLOCKWISE;
}
VkPipeline
APITest::VulkanGraphicsPipeline::get(APITest::VulkanRenderPass *pass) {

  auto *colorPass = dynamic_cast<VulkanColorPass *>(pass);

  if (!colorPass)
    throw std::runtime_error("[VULKAN][ERROR]: trying to bind graphics "
                             "pipeline to non-color render pass.");

  auto vkPass = colorPass->get();
  if (perPassMap_.count(vkPass))
    return perPassMap_.at(vkPass);
  auto &instance = *parent_->parent_;

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
      initializers::pipelineInputAssemblyStateCreateInfo(
          VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
  VkPipelineRasterizationStateCreateInfo rasterizationState =
      initializers::pipelineRasterizationStateCreateInfo(
          VK_POLYGON_MODE_FILL,
          convertCullMode(layout_.rasterizerLayout.cullingState.cullMode),
          convertFrontFace(layout_.rasterizerLayout.cullingState.face), 0);
  VkPipelineColorBlendAttachmentState blendAttachmentState{};
  if (layout_.blendingState.enable) {
    // Enable blending
    blendAttachmentState;
    blendAttachmentState.blendEnable = VK_TRUE;
    blendAttachmentState.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachmentState.dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachmentState.srcAlphaBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

  } else
    blendAttachmentState =
        initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);

  VkPipelineColorBlendStateCreateInfo colorBlendState =
      initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

  if (layout().rasterizerLayout.depthTest.enable &&
      !colorPass->hasDepthBuffer())
    throw std::runtime_error(
        "[VULKAN][ERROR]: binding graphics pipeline with depth test enabled to "
        "color pass without depth buffer allocated.");

  VkPipelineDepthStencilStateCreateInfo depthStencilState =
      initializers::pipelineDepthStencilStateCreateInfo(
          layout().rasterizerLayout.depthTest.enable ? VK_TRUE : VK_FALSE,
          layout().rasterizerLayout.depthTest.write ? VK_TRUE : VK_FALSE,
          convertToVkCompareOp(layout().rasterizerLayout.depthTest.compOp));
  VkPipelineViewportStateCreateInfo viewportState =
      initializers::pipelineViewportStateCreateInfo(1, 1, 0);
  VkPipelineMultisampleStateCreateInfo multisampleState =
      initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT,
                                                       0);
  std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT,
                                                     VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState =
      initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables, 0);
  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
  VkGraphicsPipelineCreateInfo pipelineCI =
      initializers::pipelineCreateInfo(layoutHandle, vkPass, 0);
  pipelineCI.pInputAssemblyState = &inputAssemblyState;
  pipelineCI.pRasterizationState = &rasterizationState;
  pipelineCI.pColorBlendState = &colorBlendState;
  pipelineCI.pMultisampleState = &multisampleState;
  pipelineCI.pViewportState = &viewportState;
  pipelineCI.pDepthStencilState = &depthStencilState;
  pipelineCI.pDynamicState = &dynamicState;
  pipelineCI.stageCount = shaderStages.size();
  pipelineCI.pStages = shaderStages.data();

  auto [stride, attrDesc] = vertexStateCI(layout_);

  VkPipelineVertexInputStateCreateInfo ret{};

  VkVertexInputBindingDescription vertexAttrBindingInfo =
      VkVertexInputBindingDescription({0, stride, VK_VERTEX_INPUT_RATE_VERTEX});

  ret.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  ret.vertexAttributeDescriptionCount = attrDesc.size();

  if (attrDesc.empty()) {
    ret.pVertexAttributeDescriptions = nullptr;
  } else {
    ret.pVertexAttributeDescriptions = attrDesc.data();
  }

  if (stride == 0) {
    ret.pVertexBindingDescriptions = nullptr;
    ret.vertexBindingDescriptionCount = 0;

  } else {
    ret.pVertexBindingDescriptions = &vertexAttrBindingInfo;
    ret.vertexBindingDescriptionCount = 1;
  }

  pipelineCI.pVertexInputState = &ret;
  pipelineCI.subpass = 0;

  // binding vertex shader

  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module =
      instance.getShaderManager()->getModule(layout_.vertexLayout.vertexShader);
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].pName = "main";

  // binding fragment shader

  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module = instance.getShaderManager()->getModule(
      layout_.fragmentLayout.fragmentShader);
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].pName = "main";

  VkPipeline newPipeline;

  VK_CHECK_RESULT(vkCreateGraphicsPipelines(
      instance.getVulkanDevice()->get(), parent_->pipelineCache, 1, &pipelineCI,
      nullptr, &newPipeline));

  perPassMap_.insert_or_assign(vkPass, newPipeline);

  return newPipeline;
}

APITest::VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {}

APITest::VulkanPipeline::~VulkanPipeline() {
  auto device = parent_->parent_->getVulkanDevice()->get();

  for (auto &pipeline : perPassMap_)
    vkDestroyPipeline(device, pipeline.second, nullptr);

  vkDestroyPipelineLayout(device, layoutHandle, nullptr);
}
