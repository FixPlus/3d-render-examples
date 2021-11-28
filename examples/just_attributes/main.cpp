/**
 * ------------------------------ Just attributes
 * ---------------------------------
 *
 * This example demonstrates the basics of rendering in 3D space. It doesn't use
 * anything more complex than Vertex Attributes - easiest way to pass arbitrary
 * information to the shader. Although such method shows it's inefficiency - all
 * vertex calculation must be performed on CPU.
 *
 * Another thing this example introduce is depth buffer(aka Z-buffer). It is
 * image attachment similar to color attachment but used to store not color
 * information (ouFragColor), but information about fragment's depth. It is
 * common way to "sort" drawn triangles in graphics - that more close one(less
 * Z-coordinate) will override more distant one. In contraire to OpenGL, Vulkan
 * API doesn't have default depth buffer and it should be explicitly created if
 * needed. Downside of using depth buffer is additional memory read/write
 * operations that cost perfomance downgrade. But in fact most of the time it's
 * more cheap than sorting triangle arrays on CPU.
 *
 * */

#include "common.h"
#include <random>

struct PerVertexAttributes {
  glm::vec4 pos{glm::vec3{0.0f}, 1.0f};
  glm::vec4 normal{glm::vec3{0.0f, 1.0f, 0.0f}, 0.0f};
  glm::vec3 color{1.0f, 0.0f, 0.0f};
};

class Tetra {

  static std::array<PerVertexAttributes, 12> vertices;

  APITest::VertexBufferRef vertexBuffer{};

  glm::mat4 transform;

public:
  static APITest::PipelineRef pipeline;

  glm::vec3 color;

  glm::vec3 rotAxis;
  float revPerSecond;
  glm::vec3 translate;
  glm::vec3 scale;

  Tetra(APITest::RenderInterfaceRef const &renderer, glm::vec3 col,
        glm::vec3 translate_ = glm::vec3{0.0f},
        std::pair<float, glm::vec3> rotation = {1.0f, {0.0f, 1.0f, 0.0f}},
        glm::vec3 scale_ = glm::vec3{1.0f})
      : color{col}, translate(translate_), scale(scale_),
        rotAxis(rotation.second), revPerSecond(rotation.first) {
    vertexBuffer = renderer->createVertexBuffer(
        vertices.data(), vertices.size() * sizeof(PerVertexAttributes),
        APITest::MemoryType::HOST_VISIBLE);
  }

  void update(float time) {
    glm::mat4 rotate =
        glm::rotate(glm::mat4(1.0f), glm::radians(360.0f * time * revPerSecond),
                    glm::normalize(rotAxis));
    transform =
        glm::scale(glm::translate(glm::mat4(1.0f), translate) * rotate, scale);
  }

  void draw(APITest::CommandRecorder *recorder, Camera const &camera) const {
    std::array<PerVertexAttributes, 12> stagedAttributes;
    static_assert(sizeof(stagedAttributes) == sizeof(PerVertexAttributes) * 12);
    auto perspective = camera.getCameraMatrix();
    std::transform(vertices.begin(), vertices.end(), stagedAttributes.begin(),
                   [this, perspective](PerVertexAttributes const &attr) {
                     PerVertexAttributes ret;
                     ret.normal = perspective * transform * attr.normal;
                     ret.pos = perspective * transform * attr.pos;
                     return ret;
                   });
    std::transform(stagedAttributes.begin(), stagedAttributes.end(),
                   stagedAttributes.begin(),
                   [this](PerVertexAttributes const &attr) {
                     PerVertexAttributes ret = attr;
                     ret.color = color;
                     return ret;
                   });

    vertexBuffer->push(&stagedAttributes, sizeof(stagedAttributes), 0);

    recorder->bindVertexBuffer(vertexBuffer.get());
    recorder->draw(12, 0);
  }

  static void initTetra() {
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
  };
};

std::array<PerVertexAttributes, 12> Tetra::vertices;
APITest::PipelineRef Tetra::pipeline;

Tetra generateTetra(APITest::RenderInterfaceRef const &renderer) {
  return Tetra{renderer,
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
EXAMPLE_MAIN_HEADER(Just Attributes)

APITest::GraphicsPipelineLayout tetraLayout;
tetraLayout.vertexLayout.perVertexAttribute = {
    APITest::VertexLayout::Attribute::RGBA32SF,
    APITest::VertexLayout::Attribute::RGBA32SF,
    APITest::VertexLayout::Attribute::RGB32SF};

tetraLayout.vertexLayout.vertexShader = renderer->createShaderProgram(
    {use_vulkan ? ROOT_SHADERS_DIR "just_attributes/attributes.vert.spv"
                : ROOT_SHADERS_DIR "just_attributes/attributes.vert",
     APITest::ShaderStage::VERTEX});
tetraLayout.fragmentLayout.fragmentShader = renderer->createShaderProgram(
    {use_vulkan ? ROOT_SHADERS_DIR "just_attributes/attributes.frag.spv"
                : ROOT_SHADERS_DIR "just_attributes/attributes.frag",
     APITest::ShaderStage::FRAGMENT});

// In this example we render in 3D space, so we will need depth testing in order
// to proper "sort" object by depth. Try to disable to see difference.
tetraLayout.rasterizerLayout.depthTest.enable = true;
tetraLayout.rasterizerLayout.depthTest.write = true;

Tetra::initTetra();

Tetra::pipeline = renderer->createGraphicsPipeline(tetraLayout);

auto renderPass = renderer -> createOnscreenColorPass();

std::vector<Tetra> tetras;
Camera camera{!use_vulkan /* Flip Y coordinate. Vulkan Framebuffer is Y-flipped compared to OpenGL*/};

for (int i = 0; i < 500; i++) {
  tetras.emplace_back(generateTetra(renderer));
}

renderPass->commands = [&tetras, &camera](APITest::CommandRecorder *recorder) {
  recorder->bindPipeline(Tetra::pipeline.get());
  for (auto const &tetra : tetras)
    tetra.draw(recorder, camera);
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

constexpr const double rotSpeed = 0.1f;
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

Tetra::pipeline.reset();

return 0;
}
