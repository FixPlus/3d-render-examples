//
// Created by Бушев Дмитрий on 07.11.2021.
//

#ifndef RENDERAPITEST_OGLRENDERIMPL_H
#define RENDERAPITEST_OGLRENDERIMPL_H

#include "RenderInterface.h"
#include "common.h"
#include <GL/glew.h>
#include <map>

namespace APITest {

class OGLShaderManager;
class OGLPipelineManager;
class OGLRenderJob;
class WindowInterface;
class GuiInterface;

class OGLRenderImpl final : public RenderInterface {
  WindowRef window_;
  WindowInterface *windowInterface = nullptr;
  GUIRef gui_ = nullptr;
  GuiInterface *guiInterface = nullptr;
  OGLShaderManager *shaderManager = nullptr;
  OGLPipelineManager *pipelineManager = nullptr;
  OGLRenderJob *renderJob = nullptr;
  StatisticsRecorder statRecorder;

  void setupCallbacks();

public:
  bool isDefaultTarget(ImageRef imageRef) const;

  OGLPipelineManager *getPipelineManager() const { return pipelineManager; }
  GuiInterface *getGuiInterface() const { return guiInterface; }
  explicit OGLRenderImpl(WindowRef window);

  void connectWindow(WindowRef &&window) override;

  ImageRef createImage(ImageDesc desc) override;

  SamplerRef createSampler(SamplerDesc const &desc) override;

  UniformBufferRef
  createUniformBuffer(void *initialData, size_t initialSize,
                      MemoryType = MemoryType::HOST_VISIBLE) override;

  DescriptorSetLayoutRef
  createDescriptorLayout(std::vector<DescriptorLayout> const &desc) override;

  GraphicsPipelineRef
  createGraphicsPipeline(GraphicsPipelineLayout layout) override;

  ShaderRef createShaderProgram(ShaderDesc const &shaderDesc) override;

  VertexBufferRef
  createVertexBuffer(void *initialData, size_t initialSize,
                     MemoryType = MemoryType::HOST_VISIBLE) override;
  IndexBufferRef createIndexBuffer(
      void *initialData, size_t initialSize,
      IndexBuffer::Type indexType = IndexBuffer::Type::INDEX_TYPE_UINT_32,
      MemoryType = MemoryType::HOST_VISIBLE) override;

  void pushNewRenderPassGraph(RenderPassRef node) override;

  OnscreenRenderPassRef createOnscreenColorPass() override;

  WindowRef getWindow() const override;

  GUIRef getGUI() override;

  void waitIdle() override;
  bool render() override;

  FrameStatistics const &statistics() const override {
    return statRecorder.stat();
  };

  StatisticsRecorder &recorder() { return statRecorder; }

  ~OGLRenderImpl() override;
};

} // namespace APITest
#endif // RENDERAPITEST_OGLRENDERIMPL_H
