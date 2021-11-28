//
// Created by Бушев Дмитрий on 27.10.2021.
//

#ifndef RENDERAPITEST_RENDERINTERFACE_H
#define RENDERAPITEST_RENDERINTERFACE_H
#include "keycodes.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace APITest {

class Pipeline;
using PipelineRef = std::unique_ptr<Pipeline>;

struct Window;
using WindowRef = std::shared_ptr<Window>;

using ShaderRef = uint32_t;

struct Sampler {

  virtual ~Sampler() = default;
};
struct SamplerDesc {
  // TODO
};
using SamplerRef = std::shared_ptr<Sampler>;

struct Image;
using ImageRef = std::shared_ptr<Image>;

struct UniformBuffer;
using UniformBufferRef = std::shared_ptr<UniformBuffer>;

enum class ShaderStage { VERTEX, FRAGMENT };

using CombinedImageSampler = std::pair<SamplerRef, ImageRef>;

struct UniformDescriptor {
  int binding;
  ShaderStage shaderStage;
  std::variant<CombinedImageSampler, UniformBufferRef> descriptor;
};

struct CommandRecorder;
/** @brief This is complete description of shader program global input layout.
 *
 *  Global variables that are shared across multiple primitives(vertices or
 * fragments) are called uniforms.
 *
 *  OpenGL 4.2+ core spec allows to have almost identical to Vulkan uniform
 * layout declarations in GLSL.
 *
 *  This wrapper doesn't use global uniforms that are specific for one of the
 * APIs (e.g. PushConstants in Vulkan and non-block non-opaque uniforms in
 * OpenGL). Neither it uses Vulkan capability to have multiple descriptor sets
 * (TODO: actually, this would be nice to support.)
 *
 *  */

struct UniformDescriptorSet {

  // TODO: maybe some util methods here...

  virtual ~UniformDescriptorSet() = default;
};

using UniformDescriptorSetRef = std::shared_ptr<UniformDescriptorSet>;

struct DescriptorSetLayout {

  virtual UniformDescriptorSetRef allocateNewSet(UniformDescriptor *descriptors,
                                                 int count) = 0;

  virtual ~DescriptorSetLayout() = default;
};

using DescriptorSetLayoutRef = std::shared_ptr<DescriptorSetLayout>;

struct DescriptorLayout {
  int binding;
  ShaderStage stage;
  enum class Type { UNIFORM_BUFFER, COMBINED_IMAGE_SAMPLER } type;
};

struct VertexLayout {
  enum Attribute { RGBA8UNORM, RGBA32SF, RGB32SF, RG32SF, R32SF, MAT4F };
  std::vector<Attribute> perVertexAttribute{};
  ShaderRef vertexShader;
};

struct FragmentLayout {
  ShaderRef fragmentShader;
};

struct RasterizerLayout {
  struct DepthTest {
    bool enable = false;
    bool write = false;
    enum class CompOp {
      LESS_OR_EQUAL,
      LESS,
      GREATER,
      GREATER_OR_EQUAL
    } compOp = CompOp::LESS_OR_EQUAL;
  } depthTest;

  struct CullingState {
    enum FrontFace { FRONT_FACE_CW, FRONT_FACE_CCW } face = FRONT_FACE_CCW;
    enum CullMode {
      CULL_NONE,
      CULL_FRONT_FACE,
      CULL_BACK_FACE
    } cullMode = CULL_BACK_FACE;
  } cullingState;
};
struct BlendingState {
  bool enable = false;
};

struct GraphicsPipelineLayout {
  VertexLayout vertexLayout;
  RasterizerLayout rasterizerLayout;
  FragmentLayout fragmentLayout;
  DescriptorSetLayoutRef descriptorsLayout = nullptr;
  BlendingState blendingState;
};

struct ShaderDesc {
  enum class Stage { VERTEX, FRAGMENT };
  std::string filename;
  ShaderStage stage;
};

struct Pipeline {

  virtual ~Pipeline() = default;
};

struct GraphicsPipeline : virtual public Pipeline {
  virtual GraphicsPipelineLayout const &layout() const = 0;
};

using GraphicsPipelineRef = std::unique_ptr<GraphicsPipeline>;

struct ComputePipeline : virtual public Pipeline {
  // TODO: to be implemented yet
};

struct BufferBase {

  virtual size_t size() const = 0;
  virtual void push(const void *data, size_t size, size_t offset) = 0;

  virtual ~BufferBase() = default;
};

enum class MemoryType { HOST_COHERENT, HOST_VISIBLE, GPU_PRIVATE };

struct UniformBuffer : virtual public BufferBase {};

struct VertexBuffer : virtual public BufferBase {};

struct IndexBuffer : virtual public BufferBase {
  enum class Type { INDEX_TYPE_UINT_8, INDEX_TYPE_UINT_16, INDEX_TYPE_UINT_32 };
};

using IndexBufferRef = std::shared_ptr<IndexBuffer>;

struct Image {
  struct Extents {
    uint32_t width, height, depth, layers;
  };
  enum UsageBits {
    USAGE_COPY_FROM = 0x1,
    USAGE_COPY_TO = 0x2,
    USAGE_SAMPLED = 0x4,
    USAGE_INPUT_ATTACHMENT = 0x8,
    USAGE_COLOR_ATTACHMENT = 0x10,
    USAGE_DEPTH_STENCIL_ATTACHMENT = 0x20,
  };
  using Usage = uint32_t;
  enum class Type { TEXTURE_1D, TEXTURE_2D, TEXTURE_3D, INVALID };
  enum class Format { RGBA8, D24S8, R11G11B10, INVALID };

  virtual void load(void const *data) = 0;
  virtual Type getType() const = 0;
  virtual Format getFormat() const = 0;
  virtual Extents getImageExtents() const = 0;
  virtual uint32_t arrayLayers() const = 0;
  virtual Usage getUsage() const = 0;
  bool isDepth() const { return getFormat() == Format::D24S8; }

  virtual ~Image() = default;
};

struct ImageDesc {
  Image::Extents extents;
  Image::Type type;
  Image::Format format;
  Image::Usage usage;
  MemoryType memType;
};

using VertexBufferRef = std::unique_ptr<VertexBuffer>;

struct CommandRecorder {

  virtual void bindPipeline(Pipeline *pipeline) = 0;
  virtual void bindVertexBuffer(VertexBuffer *, uint32_t binding = 0) = 0;
  virtual void bindIndexBuffer(IndexBuffer *buffer) = 0;
  virtual void draw(uint32_t vertexCount,
                    uint32_t firstIndex) = 0; // don't use index buffer
  virtual void drawIndexed(uint32_t indexCount, uint32_t firstIndex,
                           uint32_t vertexOffset) = 0; // uses index buffer
  virtual void bindDescriptorSet(UniformDescriptorSet *set) = 0;

  virtual ~CommandRecorder() = default;
};

struct RenderPass;
using RenderPassRef = std::weak_ptr<RenderPass>;

struct RenderPass {
  std::function<void(CommandRecorder *const)> commands;
  std::vector<RenderPassRef> dependencies;

  virtual ~RenderPass() = default;
};

struct ColorPass : virtual public RenderPass {

  virtual Image::Extents getFramebufferExtents() const = 0;
};

struct OffscreenRenderPass : virtual public ColorPass {
  /** Defines what to do with attachment contents in the beginning of the pass.
   * Defaults to CLEAR. */
  enum class LoadOp { LOAD, CLEAR, DONT_CARE };
  /** Defines what to do with attachment contents after the pass finishes.
   * Defaults to STORE. */
  enum class StoreOp { STORE, DONT_CARE };

  virtual void setLoadOp(uint32_t binding, LoadOp op) = 0;
  virtual void setStoreOp(uint32_t binding, StoreOp op) = 0;

  virtual ImageRef createDepthBuffer(uint32_t binding,
                                     Image::Format format) = 0;
  virtual ImageRef createColorBuffer(uint32_t binding,
                                     Image::Format format) = 0;
};

struct OnscreenRenderPass : virtual public ColorPass {
  /** Set color buffer binding index to custom value. Defaults 0. */
  virtual void setColorBufferIndex(uint32_t binding) = 0;
  /** Depth buffer is not created by default(in Vulkan). Enable if needed on
   * custom binding index. */
  virtual void enableDepthBuffer(uint32_t binding) = 0;
};

/** Wrapper struct to operate pass in handy. */

struct RenderPassRefBase {
protected:
  std::shared_ptr<RenderPass> pass_;

public:
  explicit RenderPassRefBase(std::shared_ptr<RenderPass> pass)
      : pass_(std::move(pass)) {}

  operator RenderPassRef() { return pass_; };
};

template <typename T> struct ChildRenderPassRef : public RenderPassRefBase {

  explicit ChildRenderPassRef(T *allocPass)
      : RenderPassRefBase(std::shared_ptr<RenderPass>(allocPass)) {}

  T *operator->() { return dynamic_cast<T *>(pass_.get()); }

  T const *operator->() const { return dynamic_cast<T *>(pass_.get()); }
};

using OnscreenRenderPassRef = ChildRenderPassRef<OnscreenRenderPass>;
using OffscreenRenderPassRef = ChildRenderPassRef<OffscreenRenderPass>;

class GUI;

using GUIRef = std::shared_ptr<GUI>;

struct FrameStatistics {
  /** ------------------ immediate parameters ------------------------- */

  double overallTime = 0.0;   // in seconds
  double lastFrameTime = 0.0; // in seconds

  /** ------------ Average evaluated frame stat ----------------------- */

  /**  { block name, {block time(ms), % out of total frame time}} */
  struct TimeBlock {
    std::string name;
    std::pair<double, double> duration;
    std::vector<std::unique_ptr<TimeBlock>> children{};

    TimeBlock(std::string nm = "", std::pair<double, double> dur = {.0, .0})
        : name(std::move(nm)), duration(std::move(dur)){};

  } frameTimeBlock;

  /** @brief custom named metrics. */
  std::vector<std::pair<std::string, float>> doubleMetrics;
  std::vector<std::pair<std::string, int>> integerMetrics;
};

class RenderInterface {
public:
  /** @brief displays render performance metrics.*/
  virtual FrameStatistics const &statistics() const = 0;

  /** @brief cannot connect more than one window. Previous window will be
   * destroyed.
   *  @return reference for the window surface image. */
  virtual void connectWindow(WindowRef &&window) = 0;

  virtual WindowRef getWindow() const = 0;

  /** @brief create as much images as you wish, but beware of exceeding GPU
   * memory limits. */
  virtual ImageRef createImage(ImageDesc desc) = 0;

  virtual GraphicsPipelineRef
  createGraphicsPipeline(GraphicsPipelineLayout layout) = 0;

  virtual VertexBufferRef
  createVertexBuffer(void *initialData, size_t initialSize,
                     MemoryType = MemoryType::HOST_VISIBLE) = 0;
  virtual IndexBufferRef createIndexBuffer(
      void *initialData, size_t initialSize,
      IndexBuffer::Type indexType = IndexBuffer::Type::INDEX_TYPE_UINT_32,
      MemoryType = MemoryType::HOST_VISIBLE) = 0;

  virtual DescriptorSetLayoutRef
  createDescriptorLayout(std::vector<DescriptorLayout> const &desc) = 0;

  /** @brief loads and compiles shader program */
  virtual ShaderRef createShaderProgram(ShaderDesc const &shaderDesc) = 0;

  virtual SamplerRef createSampler(SamplerDesc const &desc) = 0;

  virtual UniformBufferRef
  createUniformBuffer(void *initialData, size_t initialSize,
                      MemoryType = MemoryType::HOST_VISIBLE) = 0;

  /** @brief overrides previous render pass graph. */
  virtual void pushNewRenderPassGraph(RenderPassRef node) = 0;

  virtual OnscreenRenderPassRef createOnscreenColorPass() = 0;

  virtual GUIRef getGUI() = 0;

  /** @brief complete render task.
   *  @return true on success. */
  virtual bool render() = 0;

  virtual void waitIdle() = 0;

  virtual ~RenderInterface() = default;
};

using RenderInterfaceRef = std::unique_ptr<RenderInterface>;
RenderInterfaceRef createVulkanAPI(const WindowRef &window = nullptr,
                                   bool enableValidation = false);
RenderInterfaceRef createOpenGLAPI(WindowRef window = nullptr);
bool isVulkan(RenderInterface const *api);
bool isOpenGL(RenderInterface const *api);

struct WindowDesc {
  uint32_t width, height;
  std::string title;
};

struct Window {

  /** Window surface framebuffer controls and queries. */
  virtual uint32_t width() const = 0;
  virtual uint32_t height() const = 0;
  virtual void resize(uint32_t width, uint32_t height) = 0;
  std::function<void(int, int)> onFramebufferResize = [](int, int) {};
  /** @return check if window is minimized/iconified. */
  virtual bool minimized() const = 0;

  /** Screen cursor controls and queries. */
  virtual bool cursorEnabled() const = 0;
  virtual void disableCursor() = 0;
  virtual void enableCursor() = 0;

  /** @brief must be called consistently in order to process window events
   * properly and not cause it to hang.
   *  @return true if window hits close condition (e.g. close button clicked).
   * After that window surface becomes invalid and neither this method nor
   * render() must not be called till window is recreated.
   * */
  virtual bool handleEvents() = 0;
};

struct GUI {

  /** ----------------------------- INPUT --------------------------------*/
  /** --------------------------------------------------------------------*/

  /** -------------------------- keyboard state --------------------------*/

  /** @brief configurable keyboard callbacks */
  std::function<void(int)> onKeyDown = [](int) {};
  std::function<void(int)> onKeyUp = [](int) {};
  std::function<void(int)> onKeyPress = [](int) {};

  /** @brief fixed function keyboard state queries. */
  virtual bool isKeyPressed(int) const = 0;

  /** ---------------------------- mouse state ---------------------------*/

  /** @brief configurable mouse callbacks */
  std::function<void(double, double)> onMouseMove = [](double, double) {};
  std::function<void(int)> onMouseButtonDown = [](int) {};
  std::function<void(int)> onMouseButtonUp = [](int) {};
  std::function<void(double)> onMouseScroll = [](double) {};

  /** @brief fixed function mouse state queries. */
  virtual int xCurPos() const = 0;
  virtual int yCurPos() const = 0;
  virtual bool leftMouseButtonPressed() const = 0;
  virtual bool rightMouseButtonPressed() const = 0;
  virtual bool middleMouseButtonPressed() const = 0;

  /** ----------------------------- OVERLAY ------------------------------*/
  /** --------------------------------------------------------------------*/

  // TODO: implement overlay

  virtual bool isEnabled() const = 0;
  virtual void toggle() = 0;

  virtual ~GUI() = default;
};

WindowRef createGLFWWindow(const WindowDesc &desc);

} // namespace APITest
#endif // RENDERAPITEST_RENDERINTERFACE_H
