//
// Created by Бушев Дмитрий on 13.11.2021.
//

#include "IGGuiImpl.h"
#include "imgui/imgui.h"
#include "keycodes.h"
#include <array>
#include <glm/glm.hpp>
#include <iostream>
#include <stack>

namespace APITest {
int IGGuiImpl::instanceCounter = 0;

ImVec4 operator+(ImVec4 const &lhs, ImVec4 const &rhs) {
  ImVec4 ret;
  ret.x = lhs.x + rhs.x;
  ret.y = lhs.y + rhs.y;
  ret.z = lhs.z + rhs.z;
  ret.w = lhs.w + rhs.w;
  return ret;
}
template <typename T> ImVec4 operator*(ImVec4 const &lhs, T const &rhs) {
  ImVec4 ret;
  ret.x = lhs.x * rhs;
  ret.y = lhs.y * rhs;
  ret.z = lhs.z * rhs;
  ret.w = lhs.w * rhs;
  return ret;
}

void IGGuiImpl::InitImGui() {
  instanceCounter++;
  if (instanceCounter > 1)
    throw std::runtime_error(
        "[GUI:ImGui][ERROR]: cannot create more than one instance of ImGui.");

  // Init ImGui
  ImGui::CreateContext();
  // Color scheme
  ImVec4 mainColor = useVulkan ? ImVec4(1.0f, 0.0f, 0.0f, 0.0f)
                               : ImVec4(0.0f, 0.0f, 1.0f, 0.0f);
  ImVec4 alpha = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
  ImGuiStyle &style = ImGui::GetStyle();
  style.Colors[ImGuiCol_TitleBg] = mainColor + alpha;
  style.Colors[ImGuiCol_TitleBgActive] = mainColor + alpha;
  style.Colors[ImGuiCol_TitleBgCollapsed] = mainColor + alpha * 0.1f;
  style.Colors[ImGuiCol_MenuBarBg] = mainColor + alpha * 0.4f;
  style.Colors[ImGuiCol_Header] = mainColor * 0.8f + alpha * 0.4f;
  style.Colors[ImGuiCol_HeaderActive] = mainColor + alpha * 0.4f;
  style.Colors[ImGuiCol_HeaderHovered] = mainColor + alpha * 0.4f;
  style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
  style.Colors[ImGuiCol_CheckMark] = mainColor + alpha * 0.8f;
  style.Colors[ImGuiCol_SliderGrab] = mainColor + alpha * 0.4f;
  style.Colors[ImGuiCol_SliderGrabActive] = mainColor + alpha * 0.8f;
  style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
  style.Colors[ImGuiCol_Button] = mainColor + alpha * 0.4f;
  style.Colors[ImGuiCol_ButtonHovered] = mainColor + alpha * 0.6f;
  style.Colors[ImGuiCol_ButtonActive] = mainColor + alpha * 0.8f;
  // Dimensions
  ImGuiIO &io = ImGui::GetIO();
  io.FontGlobalScale = 1.0;

  io.KeyMap[ImGuiKey_Tab] = KEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow] = KEY_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = KEY_PAGE_UP;
  io.KeyMap[ImGuiKey_PageDown] = KEY_PAGE_DOWN;
  io.KeyMap[ImGuiKey_Home] = KEY_HOME;
  io.KeyMap[ImGuiKey_End] = KEY_END;
  io.KeyMap[ImGuiKey_Insert] = KEY_INSERT;

  io.KeyMap[ImGuiKey_Delete] = KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = KEY_BACKSPACE;
  io.KeyMap[ImGuiKey_Space] = KEY_SPACE;
  io.KeyMap[ImGuiKey_Enter] = KEY_ENTER;
  io.KeyMap[ImGuiKey_Escape] = KEY_ESCAPE;
  io.KeyMap[ImGuiKey_A] = KEY_A; // for text edit CTRL+A: select all
  io.KeyMap[ImGuiKey_C] = KEY_C; // for text edit CTRL+C: copy
  io.KeyMap[ImGuiKey_V] = KEY_V; // for text edit CTRL+V: paste
  io.KeyMap[ImGuiKey_X] = KEY_X; // for text edit CTRL+X: cut
  io.KeyMap[ImGuiKey_Y] = KEY_Y; // for text edit CTRL+Y: redo
  io.KeyMap[ImGuiKey_Z] = KEY_Z; // for text edit CTRL+Z: undo
}

IGGuiImpl::IGGuiImpl(RenderInterface *renderInterface)
    : GuiInterface(renderInterface), useVulkan(isVulkan(renderInterface)) {
  InitImGui();

  auto &io = ImGui::GetIO();

  // Create font texture
  unsigned char *fontData;
  int texWidth, texHeight;

  auto fontFilename = "Roboto-Medium.ttf";

  // io.Fonts->AddFontFromFileTTF(fontFilename, 16.0f, nullptr,
  // io.Fonts->GetGlyphRangesCyrillic());

  io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
  ImageDesc fontTextureDesc{};
  fontTextureDesc.memType = MemoryType::GPU_PRIVATE;
  fontTextureDesc.format = Image::Format::RGBA8;
  fontTextureDesc.extents = {static_cast<uint32_t>(texWidth),
                             static_cast<uint32_t>(texHeight), 1, 1};
  fontTextureDesc.type = Image::Type::TEXTURE_2D;
  fontTextureDesc.usage = Image::USAGE_SAMPLED | Image::USAGE_COPY_TO;

  fontTexture_ = renderInterface->createImage(fontTextureDesc);
  fontTexture_->load(fontData);
  SamplerDesc samplerDesc{};
  fontSampler_ = renderInterface->createSampler(samplerDesc);

  propsBuffer_ =
      renderInterface->createUniformBuffer(nullptr, 4 * sizeof(float));

  GraphicsPipelineLayout pipelineLayout{};
  bool useVulkan = isVulkan(renderInterface);
  pipelineLayout.vertexLayout.perVertexAttribute = {
      VertexLayout::Attribute::RG32SF,
      VertexLayout::Attribute::RG32SF,
      VertexLayout::Attribute::RGBA8UNORM,
  };
  pipelineLayout.vertexLayout.vertexShader =
      renderInterface->createShaderProgram({useVulkan
                                                ? "internal/uioverlay.vert.spv"
                                                : "internal/uioverlay.vert",
                                            ShaderStage::VERTEX});
  pipelineLayout.fragmentLayout.fragmentShader =
      renderInterface->createShaderProgram({useVulkan
                                                ? "internal/uioverlay.frag.spv"
                                                : "internal/uioverlay.frag",
                                            ShaderStage::FRAGMENT});

  DescriptorLayout fontSamplerLayout{};
  fontSamplerLayout.binding = 0;
  fontSamplerLayout.type = DescriptorLayout::Type::COMBINED_IMAGE_SAMPLER;
  fontSamplerLayout.stage = ShaderStage::FRAGMENT;

  DescriptorLayout propsBufferLayout{};
  propsBufferLayout.binding = 1;
  propsBufferLayout.type = DescriptorLayout::Type::UNIFORM_BUFFER;
  propsBufferLayout.stage = ShaderStage::VERTEX;

  pipelineUniformLayout_ = renderInterface->createDescriptorLayout(
      {fontSamplerLayout, propsBufferLayout});
  pipelineLayout.descriptorsLayout = pipelineUniformLayout_;

  pipelineLayout.rasterizerLayout.cullingState.cullMode =
      RasterizerLayout::CullingState::CULL_NONE;
  pipelineLayout.blendingState.enable = true;

  pipeline_ = renderInterface->createGraphicsPipeline(pipelineLayout);

  std::array<UniformDescriptor, 2> fontSamplerDescriptor;
  fontSamplerDescriptor[0].binding = 0;
  fontSamplerDescriptor[0].descriptor =
      std::pair<SamplerRef, ImageRef>{fontSampler_, fontTexture_};
  fontSamplerDescriptor[0].shaderStage = ShaderStage::FRAGMENT;

  fontSamplerDescriptor[1].binding = 1;
  fontSamplerDescriptor[1].descriptor = propsBuffer_;
  fontSamplerDescriptor[1].shaderStage = ShaderStage::VERTEX;

  fontDescriptorSet_ =
      pipelineUniformLayout_->allocateNewSet(fontSamplerDescriptor.data(), 2);
}

void IGGuiImpl::update() {
  ImGuiIO &io = ImGui::GetIO();

  io.DisplaySize = ImVec2((float)windowInterface_->width(),
                          (float)windowInterface_->height());

  if (!drawOverlay)
    return;

  ImGui::NewFrame();

  dumpStatistics();

  ImGui::EndFrame();
}

void IGGuiImpl::render() {

  if (!drawOverlay)
    return;
  ImGui::Render();

  ImDrawData *imDrawData = ImGui::GetDrawData();

  if (!imDrawData) {
    return;
  };

  // Note: Alignment is done inside buffer creation
  uint64_t vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
  uint64_t indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

  // Update buffers only if vertex or index count has been changed compared to
  // current buffer size
  if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
    return;
  }

  // Vertex buffer

  // reallocate only if not enough space
  if ((cachedVertexCount < imDrawData->TotalVtxCount)) {
    renderInterface_->waitIdle();
    vertexBuf_ =
        renderInterface_->createVertexBuffer(nullptr, vertexBufferSize);

    cachedVertexCount = imDrawData->TotalVtxCount;
  }

  // Index buffer
  VkDeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
  // reallocate only if not enough space
  if ((cachedIndexCount < imDrawData->TotalIdxCount)) {
    renderInterface_->waitIdle();
    indexBuf_ = renderInterface_->createIndexBuffer(
        nullptr, indexBufferSize, IndexBuffer::Type::INDEX_TYPE_UINT_16,
        MemoryType::HOST_VISIBLE);

    cachedIndexCount = imDrawData->TotalIdxCount;
  }
  // Upload data
  size_t vtxDst = 0;
  size_t idxDst = 0;

  for (int n = 0; n < imDrawData->CmdListsCount; n++) {
    const ImDrawList *cmd_list = imDrawData->CmdLists[n];
    vertexBuf_->push(cmd_list->VtxBuffer.Data,
                     cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), vtxDst);
    indexBuf_->push(cmd_list->IdxBuffer.Data,
                    cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), idxDst);
    vtxDst += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
    idxDst += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
    ;
  }
}

void IGGuiImpl::draw(CommandRecorder *commandRecorder) {
  if (!drawOverlay)
    return;
  ImDrawData *imDrawData = ImGui::GetDrawData();
  int32_t vertexOffset = 0;
  int32_t indexOffset = 0;

  if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
    return;
  }

  auto &io = ImGui::GetIO();
  auto scale = glm::vec2(2.0f / io.DisplaySize.x,
                         (useVulkan ? 1.0f : -1.0f) * 2.0f / io.DisplaySize.y);
  auto translate = glm::vec2(-1.0f, (useVulkan ? -1.0f : 1.0f));

  propsBuffer_->push(&scale, sizeof(scale), 0);
  propsBuffer_->push(&translate, sizeof(translate), sizeof(scale));

  commandRecorder->bindPipeline(pipeline_.get());
  commandRecorder->bindVertexBuffer(vertexBuf_.get());
  commandRecorder->bindIndexBuffer(indexBuf_.get());
  commandRecorder->bindDescriptorSet(fontDescriptorSet_.get());

  for (int32_t i = 0; i < imDrawData->CmdListsCount; i++) {
    const ImDrawList *cmd_list = imDrawData->CmdLists[i];

    for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {
      const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
      /*
      VkRect2D scissorRect;
      scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
      scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
      scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z -
      pcmd->ClipRect.x); scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w
      - pcmd->ClipRect.y); vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
       */
      commandRecorder->drawIndexed(pcmd->ElemCount, indexOffset, vertexOffset);
      indexOffset += pcmd->ElemCount;
    }
    vertexOffset += cmd_list->VtxBuffer.Size;
  }
}

void IGGuiImpl::keyDown(int key) {
  auto &io = ImGui::GetIO();
  io.KeysDown[key] = true;
  switch (key) {
  case KEY_LEFT_CONTROL:
  case KEY_RIGHT_CONTROL:
    io.KeyCtrl = true;
    break;
  case KEY_LEFT_ALT:
  case KEY_RIGHT_ALT:
    io.KeyAlt = true;
    break;
  case KEY_LEFT_SHIFT:
  case KEY_RIGHT_SHIFT:
    io.KeyShift = true;
    break;
  default:
    break;
  }

  if (!io.WantCaptureKeyboard)
    onKeyDown(key);
}

void IGGuiImpl::keyPressed(int key) {
  auto &io = ImGui::GetIO();

  if (!io.WantCaptureKeyboard)
    onKeyPress(key);
}

void IGGuiImpl::keyUp(int key) {
  auto &io = ImGui::GetIO();
  io.KeysDown[key] = false;
  switch (key) {
  case KEY_LEFT_CONTROL:
  case KEY_RIGHT_CONTROL:
    io.KeyCtrl = false;
    break;
  case KEY_LEFT_ALT:
  case KEY_RIGHT_ALT:
    io.KeyAlt = false;
    break;
  case KEY_LEFT_SHIFT:
  case KEY_RIGHT_SHIFT:
    io.KeyShift = false;
    break;
  default:
    break;
  }

  if (!io.WantCaptureKeyboard)
    onKeyUp(key);
}

void IGGuiImpl::mouseMoved(double xPos, double yPos) {
  auto &io = ImGui::GetIO();
  double deltaX = cachedMouseX - xPos;
  double deltaY = cachedMouseY - yPos;
  if (windowInterface_->cursorEnabled())
    io.MousePos = {static_cast<float>(xPos), static_cast<float>(yPos)};
  else
    io.MousePos = {-FLT_MAX, -FLT_MAX};

  cachedMouseX = xPos;
  cachedMouseY = yPos;
  if (!io.WantCaptureMouse)
    onMouseMove(deltaX, deltaY);
}

void IGGuiImpl::mouseBtnDown(int button) {
  auto &io = ImGui::GetIO();
  io.MouseDown[button] = true;

  if (!io.WantCaptureMouse)
    onMouseButtonDown(button);
}

void IGGuiImpl::mouseBtnUp(int button) {
  auto &io = ImGui::GetIO();
  io.MouseDown[button] = false;

  if (!io.WantCaptureMouse)
    onMouseButtonUp(button);
}

void IGGuiImpl::mouseScroll(double xoffset, double yoffset) {
  auto &io = ImGui::GetIO();
  io.MouseWheel = xoffset;
  if (!io.WantCaptureMouse)
    onMouseScroll(xoffset);
}

void IGGuiImpl::charInput(uint32_t unicode) {
  auto &io = ImGui::GetIO();
  io.AddInputCharacter(unicode);
}

IGGuiImpl::~IGGuiImpl() {
  instanceCounter--;
  ImGui::DestroyContext();
}

bool IGGuiImpl::isKeyPressed(int key) const {
  auto &io = ImGui::GetIO();
  return io.KeysDown[key];
}

int IGGuiImpl::xCurPos() const { return cachedMouseX; }

int IGGuiImpl::yCurPos() const { return cachedMouseY; }

bool IGGuiImpl::leftMouseButtonPressed() const {
  auto &io = ImGui::GetIO();
  return io.MouseDown[0];
}

bool IGGuiImpl::rightMouseButtonPressed() const {
  auto &io = ImGui::GetIO();
  return io.MouseDown[1];
}

bool IGGuiImpl::middleMouseButtonPressed() const {
  auto &io = ImGui::GetIO();
  return io.MouseDown[2];
}

void IGGuiImpl::dumpStatistics() {
  auto const &stat = renderInterface_->statistics();

  ImGui::SetNextWindowPos(ImVec2(20.0, 20.0), ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Statistics", nullptr,
                   ImGuiWindowFlags_AlwaysAutoResize |
                       ImGuiWindowFlags_NoMove)) {
    ImGui::Text("Frame time: %.2fms (%.2ffps)",
                stat.frameTimeBlock.duration.first,
                stat.frameTimeBlock.duration.second);

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Execution time", ImGuiTreeNodeFlags_Framed)) {

      std::stack<std::pair<FrameStatistics::TimeBlock const *, int>> blockStack;

      for (auto &child : stat.frameTimeBlock.children) {
        blockStack.push({child.get(), 0});
        while (!blockStack.empty()) {
          auto [currentBlock, currentChild] = blockStack.top();
          if (currentChild == 0) {
            ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            float traffic = currentBlock->duration.second / 100.0f;
            color.y *= 1.0f - traffic;
            color.z *= 1.0f - traffic;

            if (currentBlock->children.empty()) {
              ImGui::Bullet();
              ImGui::TextColored(
                  color, "%s: %.2fms (%.2f%%)", currentBlock->name.c_str(),
                  currentBlock->duration.first, currentBlock->duration.second);
              blockStack.pop();
              continue;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            bool treeNodeOpen = ImGui::TreeNode(
                currentBlock->name.c_str(), "%s: %.2fms (%.2f%%)",
                currentBlock->name.c_str(), currentBlock->duration.first,
                currentBlock->duration.second);
            ImGui::PopStyleColor();

            if (treeNodeOpen) {
              if (!currentBlock->children.empty()) {
                blockStack.top().second = currentChild + 1;
                blockStack.push(
                    {currentBlock->children.at(currentChild).get(), 0});
                continue;
              }
            } else {
              blockStack.pop();
              continue;
            }
          } else if (!currentBlock->children.empty() &&
                     currentChild != currentBlock->children.size()) {
            blockStack.top().second = currentChild + 1;
            blockStack.push({currentBlock->children.at(currentChild).get(), 0});
            continue;
          }

          ImGui::TreePop();
          blockStack.pop();
        }
      }
    }
    if (!stat.integerMetrics.empty()) {
      ImGui::Separator();
      for (auto const &metric : stat.integerMetrics) {
        ImGui::Text("%s: %d", metric.first.c_str(), metric.second);
      }
    }

    if (!stat.doubleMetrics.empty()) {
      ImGui::Separator();
      for (auto const &metric : stat.doubleMetrics) {
        ImGui::Text("%s: %.2f", metric.first.c_str(), metric.second);
      }
    }
  }

  ImGui::End();
}

} // namespace APITest
