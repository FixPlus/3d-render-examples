//
// Created by Бушев Дмитрий on 15.11.2021.
//
#include <utility>

#include "common.h"
#include "stb_image.h"

APITest::ImageRef
loadTexture(APITest::RenderInterfaceRef const &renderInterface,
            const char *filename) {
  int texWidth, texHeight, texChannels;

  stbi_uc *pixels =
      stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

  if (pixels == nullptr) {
    std::cerr << "Could not open " << filename << std::endl;
    return nullptr;
  }
  auto texture = renderInterface->createImage(
      {{static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1,
        1},
       APITest::Image::Type::TEXTURE_2D,
       APITest::Image::Format::RGBA8,
       APITest::Image::USAGE_SAMPLED | APITest::Image::USAGE_COPY_TO,
       APITest::MemoryType::GPU_PRIVATE});

  texture->load(pixels);

  return texture;
}

struct TexturedRectangle {

  struct Vertex {
    glm::vec4 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
  };

  constexpr static const std::array<Vertex, 6> vertices = {
      Vertex{{-1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
      Vertex{{1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
      Vertex{{1.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
      Vertex{{-1.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
      Vertex{{1.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
      Vertex{{-1.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
  };

  static std::array<Vertex, 6> dynamicVertices;

  static APITest::VertexBufferRef vertexBuffer;
  static APITest::SamplerRef sampler;
  static APITest::DescriptorSetLayoutRef descriptorSetLayout;
  static APITest::GraphicsPipelineRef pipeline;

  APITest::UniformDescriptorSetRef textureDescriptor;
  APITest::ImageRef texture_;

public:
  static void Init(APITest::RenderInterfaceRef const &renderer,
                   bool use_vulkan) {
    vertexBuffer = renderer->createVertexBuffer(
        dynamicVertices.data(), dynamicVertices.size() * sizeof(Vertex));
    sampler = renderer->createSampler({});
    descriptorSetLayout = renderer->createDescriptorLayout(
        {{0, APITest::ShaderStage::FRAGMENT,
          APITest::DescriptorLayout::Type::COMBINED_IMAGE_SAMPLER}});
    APITest::GraphicsPipelineLayout layout;
    layout.vertexLayout.perVertexAttribute = {
        APITest::VertexLayout::Attribute::RGBA32SF,
        APITest::VertexLayout::Attribute::RGB32SF,
        APITest::VertexLayout::Attribute::RG32SF,
    };
    layout.descriptorsLayout = descriptorSetLayout;
    layout.vertexLayout.vertexShader = renderer->createShaderProgram(
        {use_vulkan ? ROOT_SHADERS_DIR "texture/texture.vert.spv"
                    : ROOT_SHADERS_DIR "texture/texture.vert",
         APITest::ShaderStage::VERTEX});
    layout.fragmentLayout.fragmentShader = renderer->createShaderProgram(
        {use_vulkan ? ROOT_SHADERS_DIR "texture/texture.frag.spv"
                    : ROOT_SHADERS_DIR "texture/texture.frag",
         APITest::ShaderStage::FRAGMENT});

    pipeline = renderer->createGraphicsPipeline(layout);
  };

  TexturedRectangle(APITest::RenderInterfaceRef const &renderer,
                    APITest::ImageRef texture)
      : texture_(std::move(texture)) {
    APITest::UniformDescriptor desc;

    desc.binding = 0;
    desc.descriptor.emplace<APITest::CombinedImageSampler>(sampler, texture_);
    desc.shaderStage = APITest::ShaderStage::FRAGMENT;

    textureDescriptor = descriptorSetLayout->allocateNewSet(&desc, 1);
  }

  static void setDynamicVertices(glm::mat4 transform) {
    // dynamicVertices = vertices;
    std::transform(vertices.begin(), vertices.end(), dynamicVertices.begin(),
                   [transform](Vertex const &in) {
                     return Vertex{
                         transform * in.pos,
                         glm::vec3(transform * glm::vec4(in.normal, 0.0f)),
                         in.texCoord};
                   });

    vertexBuffer->push(dynamicVertices.data(),
                       dynamicVertices.size() * sizeof(Vertex), 0);
  }

  void draw(APITest::CommandRecorder *recorder) {
    recorder->bindPipeline(pipeline.get());
    recorder->bindDescriptorSet(textureDescriptor.get());
    recorder->bindVertexBuffer(vertexBuffer.get(), 0);
    recorder->draw(6, 0);
  }

  static void Terminate() {
    vertexBuffer.reset();
    sampler.reset();
    descriptorSetLayout.reset();
    pipeline.reset();
  }
};

APITest::VertexBufferRef TexturedRectangle::vertexBuffer;
APITest::SamplerRef TexturedRectangle::sampler;
APITest::DescriptorSetLayoutRef TexturedRectangle::descriptorSetLayout;
APITest::GraphicsPipelineRef TexturedRectangle::pipeline;

std::array<TexturedRectangle::Vertex, 6> TexturedRectangle::dynamicVertices =
    vertices;

EXAMPLE_MAIN_HEADER(Texture)

Camera camera{!use_vulkan};

std::vector<APITest::ImageRef> textures;
textures.emplace_back(loadTexture(renderer, "image.jpg"));
textures.emplace_back(loadTexture(renderer, "image1.jpg"));

TexturedRectangle::Init(renderer, use_vulkan);

std::vector<TexturedRectangle> texturedRectangles;
std::transform(textures.begin(), textures.end(),
               std::back_inserter(texturedRectangles),
               [&renderer](APITest::ImageRef const &texture) {
                 return TexturedRectangle{renderer, texture};
               });

int rectToDraw = 0;
int rectangleCount = texturedRectangles.size();

auto renderPass = renderer -> createOnscreenColorPass();
renderPass->commands = [&texturedRectangles,
                        &rectToDraw](APITest::CommandRecorder *recorder) {
  texturedRectangles.at(rectToDraw).draw(recorder);
};

auto gui = renderer -> getGUI();

gui->onKeyDown = [&window, &camera, &example_close, &rectToDraw, rectangleCount,
                  &gui](int key) {
  switch (key) {
  case APITest::KEY_C:
    window->cursorEnabled() ? window->disableCursor() : window->enableCursor();
    break;
  case APITest::KEY_ESCAPE:
    example_close = true;
    break;
  case APITest::KEY_O:
    gui->toggle();
    break;
  case APITest::KEY_LEFT:
    rectToDraw = rectToDraw == 0 ? rectangleCount - 1 : rectToDraw - 1;
    break;
  case APITest::KEY_RIGHT:
    rectToDraw = (rectToDraw + 1) % rectangleCount;
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

renderer->pushNewRenderPassGraph(renderPass);

MAIN_LOOP_HEAD() {
  camera.update(renderer->statistics().lastFrameTime);
  TexturedRectangle::setDynamicVertices(camera.getCameraMatrix());
}
MAIN_LOOP_TAIL()

TexturedRectangle::Terminate();

return 0;
}
