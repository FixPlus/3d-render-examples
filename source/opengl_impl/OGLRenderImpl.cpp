//
// Created by Бушев Дмитрий on 07.11.2021.
//

#include "OGLRenderImpl.h"
#include "GuiInterface.h"
#include "WindowInterface.h"
#include "opengl_impl/OGLBuffer.h"
#include "opengl_impl/OGLDescriptors.h"
#include "opengl_impl/OGLPipeline.h"
#include "opengl_impl/OGLRenderJob.h"
#include "opengl_impl/OGLSampler.h"
#include "opengl_impl/OGLShaderManager.h"
#include "opengl_impl/OGLTexture.h"
#include <cassert>
#include <iostream>

APITest::OGLRenderImpl::OGLRenderImpl(APITest::WindowRef window)
    : window_(std::move(window)),
      windowInterface(dynamic_cast<WindowInterface *>(window_.get())) {

  windowInterface->syncGLContext();

  const GLubyte *renderer = glGetString(GL_RENDERER); // get renderer string
  const GLubyte *version = glGetString(GL_VERSION);   // version as a string
  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", version);

  glewExperimental = GL_TRUE;
  glewInit();

  shaderManager = new OGLShaderManager();

  pipelineManager = new OGLPipelineManager();

  glDisable(GL_DEPTH_TEST);

  windowInterface->callback.sizeChanged = [](int width, int height) {
    glViewport(0, 0, width, height);
  };

  setupCallbacks();

  std::cout << "OpenGL context initialized successful" << std::endl;
}

void APITest::OGLRenderImpl::connectWindow(APITest::WindowRef &&window) {}

APITest::ImageRef APITest::OGLRenderImpl::createImage(ImageDesc desc) {
  // TODO
  return std::shared_ptr<Image>(new OGLTexture(desc));
}

static GLenum convertStages(APITest::ShaderStage stage) {
  switch (stage) {
  case APITest::ShaderStage::VERTEX:
    return GL_VERTEX_SHADER;
  case APITest::ShaderStage::FRAGMENT:
    return GL_FRAGMENT_SHADER;
  default:
    assert(0 && "Invalid shader stage");
  }

  return 0;
}

APITest::ShaderRef APITest::OGLRenderImpl::createShaderProgram(
    const APITest::ShaderDesc &shaderDesc) {
  return shaderManager->loadShader(shaderDesc.filename,
                                   convertStages(shaderDesc.stage));
}

bool APITest::OGLRenderImpl::render() {

  statRecorder.endFrame();
  statRecorder.beginFrame("Swap buffers");

  windowInterface->swapBuffers();

  if (gui_) {
    statRecorder.push("GUI");
    statRecorder.stamp("update");
    guiInterface->update();

    statRecorder.stamp("render");
    guiInterface->render();
    statRecorder.pop();
  }

  if (renderJob)
    renderJob->complete();

  glFlush();

  statRecorder.stamp("User Code");

  return windowInterface->handleEvents();
}

APITest::OGLRenderImpl::~OGLRenderImpl() {

  gui_.reset();

  delete pipelineManager;
  delete shaderManager;
}

bool APITest::OGLRenderImpl::isDefaultTarget(ImageRef imageRef) const {
  return imageRef == 0;
}

APITest::GraphicsPipelineRef APITest::OGLRenderImpl::createGraphicsPipeline(
    APITest::GraphicsPipelineLayout layout) {
  return pipelineManager->get(layout);
}

void APITest::OGLRenderImpl::waitIdle() {
  // TODO
}

void APITest::OGLRenderImpl::pushNewRenderPassGraph(
    APITest::RenderPassRef node) {

  delete renderJob;
  renderJob = new OGLRenderJob{this, std::move(node)};
}

APITest::OnscreenRenderPassRef
APITest::OGLRenderImpl::createOnscreenColorPass() {
  return APITest::OnscreenRenderPassRef(new OGLOnscreenRenderPass());
}

APITest::WindowRef APITest::OGLRenderImpl::getWindow() const { return window_; }

APITest::GUIRef APITest::OGLRenderImpl::getGUI() {
  if (!gui_) {
    gui_ = createImGui(this);
    guiInterface = dynamic_cast<GuiInterface *>(gui_.get());
  }

  return gui_;
}

void APITest::OGLRenderImpl::setupCallbacks() {
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

APITest::SamplerRef
APITest::OGLRenderImpl::createSampler(const APITest::SamplerDesc &desc) {

  return APITest::SamplerRef(new OGLSampler(desc));
}

APITest::DescriptorSetLayoutRef APITest::OGLRenderImpl::createDescriptorLayout(
    const std::vector<DescriptorLayout> &desc) {
  return DescriptorSetLayoutRef(new OGLDescriptorSetLayout(desc));
}

APITest::UniformBufferRef APITest::OGLRenderImpl::createUniformBuffer(
    void *initialData, size_t initialSize, APITest::MemoryType memType) {
  auto buffer =
      APITest::UniformBufferRef(new OGLUniformBuffer(memType, initialSize));

  if (initialData)
    buffer->push(initialData, initialSize, 0);

  return std::move(buffer);
}

APITest::VertexBufferRef APITest::OGLRenderImpl::createVertexBuffer(
    void *initialData, size_t initialSize, APITest::MemoryType memType) {
  auto ret =
      std::unique_ptr<VertexBuffer>(new OGLVertexBuffer(memType, initialSize));

  if (initialData != nullptr)
    ret->push(initialData, initialSize, 0);

  return std::move(ret);
}

APITest::IndexBufferRef
APITest::OGLRenderImpl::createIndexBuffer(void *initialData, size_t initialSize,
                                          APITest::IndexBuffer::Type indexType,
                                          APITest::MemoryType memType) {
  auto ret = std::unique_ptr<IndexBuffer>(
      new OGLIndexBuffer(memType, initialSize, indexType));

  if (initialData != nullptr)
    ret->push(initialData, initialSize, 0);

  return std::move(ret);
}
