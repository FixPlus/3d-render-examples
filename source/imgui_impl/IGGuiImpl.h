//
// Created by Бушев Дмитрий on 13.11.2021.
//

#ifndef RENDERAPITEST_IGGUIIMPL_H
#define RENDERAPITEST_IGGUIIMPL_H

#include "GuiInterface.h"

namespace APITest {

class IGGuiImpl final : public GuiInterface {
  static int instanceCounter;
  void InitImGui();

  ImageRef fontTexture_ = nullptr;
  SamplerRef fontSampler_ = nullptr;
  UniformBufferRef propsBuffer_ = nullptr;
  GraphicsPipelineRef pipeline_ = nullptr;
  DescriptorSetLayoutRef pipelineUniformLayout_ = nullptr;
  UniformDescriptorSetRef fontDescriptorSet_ = nullptr;
  VertexBufferRef vertexBuf_ = nullptr;
  IndexBufferRef indexBuf_ = nullptr;

  uint32_t cachedVertexCount = 0, cachedIndexCount = 0;

  int cachedMouseX = 0, cachedMouseY = 0;
  bool useVulkan = false;
  bool drawOverlay = true;

  void dumpStatistics();

public:
  explicit IGGuiImpl(RenderInterface *renderInterface);

  /** Top interface methods. */
  bool isKeyPressed(int) const override;
  int xCurPos() const override;
  int yCurPos() const override;
  bool leftMouseButtonPressed() const override;
  bool rightMouseButtonPressed() const override;
  bool middleMouseButtonPressed() const override;
  bool isEnabled() const { return drawOverlay; };
  void toggle() { drawOverlay = !drawOverlay; };

  /** Internal interface methods*/
  void update() override;
  void render() override;
  void draw(CommandRecorder *commandRecorder) override;

  void keyDown(int key) override;
  void keyPressed(int key) override;
  void keyUp(int key) override;
  void mouseMoved(double xPos, double yPos) override;
  void mouseBtnDown(int button) override;
  void mouseBtnUp(int button) override;
  ;
  void mouseScroll(double xoffset, double yoffset) override;
  void charInput(uint32_t unicode) override;

  ~IGGuiImpl() override;
};
} // namespace APITest
#endif // RENDERAPITEST_IGGUIIMPL_H
