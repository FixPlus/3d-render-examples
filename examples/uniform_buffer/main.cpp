/**
 *  ------------------------------------ Uniform Buffer
 * -------------------------------------------
 *
 *
 *  This example demonstrates usage of Uniform Buffer Objects (UBO). They are
 * used to pass arbitrary data to shader that is the same for all primitives.
 * Most commonly they are used to pass some "global" scene parameters like
 * camera and space transformation matrices.
 *
 *
 * */

#include "common.h"

struct PerVertexAttributes {
  glm::vec4 pos{glm::vec3{0.0f}, 1.0f};
  glm::vec4 normal{glm::vec3{0.0f, 1.0f, 0.0f}, 0.0f};
};

class Tetra {

  static std::array<PerVertexAttributes, 12> vertices;

  static APITest::PipelineRef pipeline;
  static APITest::VertexBufferRef vertexBuffer;
  static APITest::DescriptorSetLayoutRef tetraDescriptorLayout;

  glm::mat4 transform;

  APITest::UniformBufferRef transformMatrixBuffer = nullptr;
  APITest::UniformDescriptorSetRef descriptorSet = nullptr;

public:
  glm::vec3 color;

  glm::vec3 rotAxis;
  float revPerSecond;
  glm::vec3 translate;
  glm::vec3 scale;

  Tetra(APITest::RenderInterfaceRef const &renderer, Camera const &camera,
        glm::vec3 col, glm::vec3 translate_ = glm::vec3{0.0f},
        std::pair<float, glm::vec3> rotation = {1.0f, {0.0f, 1.0f, 0.0f}},
        glm::vec3 scale_ = glm::vec3{1.0f})
      : color{col}, translate(translate_), scale(scale_),
        rotAxis(rotation.second), revPerSecond(rotation.first) {
    transformMatrixBuffer = renderer->createUniformBuffer(
        nullptr, sizeof(glm::mat4) + sizeof(glm::vec3));
    transformMatrixBuffer->push(&transform, sizeof(transform), 0);
    transformMatrixBuffer->push(&color, sizeof(color), sizeof(transform));

    std::array<APITest::UniformDescriptor, 2> uniformDescriptors;
    uniformDescriptors[0] = {0, APITest::ShaderStage::VERTEX,
                             transformMatrixBuffer};
    uniformDescriptors[1] = {1, APITest::ShaderStage::VERTEX,
                             camera.getCameraBuffer()};
    descriptorSet = tetraDescriptorLayout->allocateNewSet(
        uniformDescriptors.data(), uniformDescriptors.size());
  }

  void update(float time) {
    glm::mat4 rotate =
        glm::rotate(glm::mat4(1.0f), glm::radians(360.0f * time * revPerSecond),
                    glm::normalize(rotAxis));
    transform =
        glm::scale(glm::translate(glm::mat4(1.0f), translate) * rotate, scale);

    transformMatrixBuffer->push(&transform, sizeof(transform), 0);
  }

  static void initTetraDraw(APITest::CommandRecorder *recorder) {
    recorder->bindPipeline(pipeline.get());
    recorder->bindVertexBuffer(vertexBuffer.get());
  }

  void draw(APITest::CommandRecorder *recorder) const {

    recorder->bindDescriptorSet(descriptorSet.get());
    recorder->draw(12, 0);
  }

  static void initTetra(APITest::RenderInterfaceRef const &renderer) {
    std::array<glm::vec4, 4> baseVertices{};
    float outerRadius = glm::sqrt(3.0f / 2.0f) / 2.0f;
    baseVertices.at(0) = glm::vec4(0.0f, outerRadius, 0.0f, 1.0f);
    float lowerPlaneOffset = glm::sqrt(2.0f / 3.0f) - outerRadius;
    float lowerPlaneOuterRadius = glm::sqrt(
        outerRadius * outerRadius - lowerPlaneOffset * lowerPlaneOffset);

    for (int i = 0; i < 3; i++) {
      float angle = glm::radians(static_cast<float>(i) * 120.0f);
      baseVertices.at(i + 1) =
          glm::vec4(lowerPlaneOuterRadius * glm::sin(angle), -lowerPlaneOffset,
                    lowerPlaneOuterRadius * glm::cos(angle), 1.0f);
    }

    for (int i = 0; i < 3; i++) {
      vertices.at(i * 3).pos = baseVertices.at(0);
      glm::vec3 firstBase = vertices.at(i * 3 + 1).pos =
          baseVertices.at((i + 1) % 3 + 1);
      glm::vec3 secondBase = vertices.at(i * 3 + 2).pos =
          baseVertices.at(i + 1);

      firstBase -= glm::vec3(baseVertices.at(0));
      secondBase -= glm::vec3(baseVertices.at(0));

      auto normal = glm::normalize(glm::cross(secondBase, firstBase));
      for (int j = 0; j < 3; j++) {
        vertices.at(i * 3 + j).normal = glm::vec4(normal, 0.0f);
      }
    }

    for (int i = 0; i < 3; i++) {
      vertices.at(9 + i).pos = baseVertices.at(i + 1);
      vertices.at(9 + i).normal = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
    }

    vertexBuffer = renderer->createVertexBuffer(
        vertices.data(), vertices.size() * sizeof(PerVertexAttributes),
        APITest::MemoryType::HOST_VISIBLE);

    APITest::GraphicsPipelineLayout tetraLayout;

    tetraLayout.vertexLayout.perVertexAttribute = {
        APITest::VertexLayout::Attribute::RGBA32SF,
        APITest::VertexLayout::Attribute::RGBA32SF};

    tetraLayout.vertexLayout.vertexShader = renderer->createShaderProgram(
        {APITest::isVulkan(renderer.get())
             ? ROOT_SHADERS_DIR "uniform_buffer/ubo.vert.spv"
             : ROOT_SHADERS_DIR "uniform_buffer/ubo.vert",
         APITest::ShaderStage::VERTEX});
    tetraLayout.fragmentLayout.fragmentShader = renderer->createShaderProgram(
        {APITest::isVulkan(renderer.get())
             ? ROOT_SHADERS_DIR "uniform_buffer/ubo.frag.spv"
             : ROOT_SHADERS_DIR "uniform_buffer/ubo.frag",
         APITest::ShaderStage::FRAGMENT});

    // In this example we render in 3D space, so we will need depth testing in
    // order to proper "sort" object by depth. Try to disable to see difference.
    tetraLayout.rasterizerLayout.depthTest.enable = true;
    tetraLayout.rasterizerLayout.depthTest.write = true;
    std::vector<APITest::DescriptorLayout> uniformLayoutDesc;
    uniformLayoutDesc.push_back(
        {0, APITest::ShaderStage::VERTEX,
         APITest::DescriptorLayout::Type::UNIFORM_BUFFER});
    uniformLayoutDesc.push_back(
        {1, APITest::ShaderStage::VERTEX,
         APITest::DescriptorLayout::Type::UNIFORM_BUFFER});

    tetraDescriptorLayout = renderer->createDescriptorLayout(uniformLayoutDesc);
    tetraLayout.descriptorsLayout = tetraDescriptorLayout;

    Tetra::pipeline = renderer->createGraphicsPipeline(tetraLayout);
  };

  static void terminateTetra() {
    vertexBuffer.reset();
    tetraDescriptorLayout.reset();
    pipeline.reset();
  }
};

std::array<PerVertexAttributes, 12> Tetra::vertices;

APITest::PipelineRef Tetra::pipeline;
APITest::VertexBufferRef Tetra::vertexBuffer;
APITest::DescriptorSetLayoutRef Tetra::tetraDescriptorLayout;

Tetra generateTetra(APITest::RenderInterfaceRef const &renderer,
                    Camera const &camera) {
  return Tetra{renderer,
               camera,
               glm::vec3{(std::rand() % 1000) / 2000.0f + 0.5f, 0.0f,
                         (std::rand() % 1000) / 2000.0f + 0.5f},
               glm::vec3((std::rand() % 1000) / 25.0f - 20.0f,
                         (std::rand() % 1000) / 25.0f - 20.0f,
                         (std::rand() % 1000) / 25.0f - 20.0f),
               std::pair<float, glm::vec3>{
                   (std::rand() % 1000) / 1000.0f * 0.4f + 0.1f,
                   glm::vec3{(std::rand() % 1000) / 500.0f - 1.0f,
                             (std::rand() % 1000) / 500.0f - 1.0f,
                             (std::rand() % 1000) / 500.0f - 1.0f}},
               glm::vec3((std::rand() % 1000) / 500.0f + 0.25f)};
}

EXAMPLE_MAIN_HEADER(Unifrom Buffer)

Camera camera{!use_vulkan};

camera.allocateCameraBuffer(renderer);

Tetra::initTetra(renderer);

std::vector<Tetra> tetras;

for (int i = 0; i < 500; i++) {
  tetras.emplace_back(generateTetra(renderer, camera));
}

auto renderPass = renderer -> createOnscreenColorPass();

renderPass->commands = [&tetras](APITest::CommandRecorder *recorder) {
  Tetra::initTetraDraw(recorder);
  for (auto const &tetra : tetras)
    tetra.draw(recorder);
};

renderPass->enableDepthBuffer(1);

renderer->pushNewRenderPassGraph(renderPass);

bool paused = false;
auto gui = renderer -> getGUI();

gui->onKeyDown = [&window, &camera, &paused, &example_close, &gui](int key) {
  switch (key) {
  case APITest::KEY_C:
    window->cursorEnabled() ? window->disableCursor() : window->enableCursor();
    break;
  case APITest::KEY_P:
    paused = !paused;
    break;
  case APITest::KEY_ESCAPE:
    example_close = true;
    break;
  case APITest::KEY_O:
    gui->toggle();
    break;
  default:;
  }

  camera.keyDown(key);
};

gui->onKeyUp = [&camera](int key) { camera.keyUp(key); };

gui->onMouseMove = [&window, &camera](double dx, double dy) {
  if (!window->cursorEnabled()) {
    camera.mouseMove(dx, dy);
  }
};

window->onFramebufferResize = [&camera](int width, int height) {
  if (height != 0) {
    camera.updateAspectRatio(static_cast<float>(width) /
                             static_cast<float>(height));
  }
};

MAIN_LOOP_HEAD() {
  camera.update(renderer->statistics().lastFrameTime);

  if (!paused)
    for (auto &tetra : tetras)
      tetra.update(renderer->statistics().overallTime);
}
MAIN_LOOP_TAIL()

Tetra::terminateTetra();

return 0;
}
