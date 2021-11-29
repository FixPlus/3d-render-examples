//
// Created by Бушев Дмитрий on 20.11.2021.
//

#ifndef RENDERAPITEST_MODEL_H
#define RENDERAPITEST_MODEL_H

#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "RenderInterface.h"
#include <filesystem>
#include <glm/detail/type_quat.hpp>
#include <glm/glm.hpp>
#include <stack>
#include <stdexcept>
#include <tiny_gltf.h>

class Camera;

namespace Examples {

enum class AttributeType {
  POSITION = 0, // 4f
  NORMAL = 1,   // 3f
  UV = 2,       // 2f
  COLOR = 3,    // 4f
  JOINT = 4,    // 4f
  WEIGHT = 5,   // 4f
  LAST = 6
};

struct Primitive {
  uint32_t firstIndex;
  uint32_t indexCount;
  uint32_t firstVertex;
  uint32_t vertexCount;

  struct BoundingBox {
    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);
    glm::vec3 size;
    glm::vec3 center;
    float radius;
  } dimensions;

  void setDimensions(glm::vec3 min, glm::vec3 max);
};

enum MeshCreateFlagsBits {
  MESH_PRE_TRANSFORM_VERTICES = 0x00000001,
  MESH_PRE_MULTIPLY_COLORS = 0x00000002,
  MESH_FLIP_Y = 0x00000004,
};

using MeshCreateFlags = uint32_t;

class MeshBase {
  APITest::VertexBufferRef vertexBuffer;
  APITest::IndexBufferRef indexBuffer;

protected:
  std::vector<Primitive> primitives_;
  std::string name_;

public:
  MeshBase(APITest::RenderInterface *renderer, tinygltf::Model const &model,
           tinygltf::Mesh const &mesh, MeshCreateFlags flags = 0,
           std::vector<AttributeType> attrMapping = {});

  void bindBuffers(APITest::CommandRecorder *recorder) const {
    recorder->bindVertexBuffer(vertexBuffer.get());
    recorder->bindIndexBuffer(indexBuffer.get());
  }
  /** bindBuffers must be called before executing this method */
  void drawPrimitive(APITest::CommandRecorder *recorder, int index) const {
    auto &primitive = primitives_.at(index);
    if (primitive.indexCount != 0)
      recorder->drawIndexed(primitive.indexCount, primitive.firstIndex,
                            primitive.firstVertex);
    else
      recorder->draw(primitive.vertexCount, primitive.firstVertex);
  };

  size_t primitiveCount() const { return primitives_.size(); };

  /** useful for camera frustum culling and collision detection. */
  Primitive::BoundingBox getPrimitiveBoundingBox(int index) const {
    return primitives_.at(index).dimensions;
  };

  std::string_view name() const { return name_; }

  virtual ~MeshBase() = default;
};

class MaterialBase {
  static APITest::ShaderRef defaultMaterialShader;

public:
  static void InitMaterialBase(APITest::RenderInterface *renderer);

  virtual std::vector<APITest::DescriptorLayout> getDescriptorLayout() const {
    return {};
  }
  virtual std::vector<APITest::UniformDescriptor> getDescriptors() const {
    return {};
  };

  virtual APITest::ShaderRef getFragmentShader() const {
    return defaultMaterialShader;
  }

  virtual APITest::RasterizerLayout getRasterizationState() const {
    APITest::RasterizerLayout::CullingState cullState;
    cullState.face = APITest::RasterizerLayout::CullingState::FRONT_FACE_CW;
    cullState.cullMode =
        APITest::RasterizerLayout::CullingState::CULL_BACK_FACE;
    APITest::RasterizerLayout ret{};
    ret.cullingState = cullState;
    ret.depthTest.enable = true;
    ret.depthTest.write = true;
    return ret;
  };

  virtual ~MaterialBase() = default;
};

using MaterialRef = std::unique_ptr<MaterialBase>;

class GLTFModel;

class MMaterial : public MaterialBase {
  APITest::UniformBufferRef materialBuffer = nullptr;

public:
  enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
  AlphaMode alphaMode = ALPHAMODE_OPAQUE;
  struct {
    float alphaCutoff = 1.0f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float pad;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
  } materialSettings;

  APITest::ImageRef *baseColorTexture = nullptr;         // binding 2
  APITest::ImageRef *metallicRoughnessTexture = nullptr; // binding 3
  APITest::ImageRef *normalTexture = nullptr;            // binding 4
  APITest::ImageRef *occlusionTexture = nullptr;         // binding 5
  APITest::ImageRef *emissiveTexture = nullptr;          // binding 6

  APITest::ImageRef *specularGlossinessTexture = nullptr; // binding 7
  APITest::ImageRef *diffuseTexture = nullptr;            // binding 8

  APITest::SamplerRef *sampler = nullptr;

  static APITest::ShaderRef fragmentShader;

  std::vector<APITest::UniformDescriptor> textureDescriptors;
  std::vector<APITest::DescriptorLayout> materialUniformLayout;

  static void InitShader(APITest::RenderInterfaceRef const &renderer);

  explicit MMaterial(APITest::RenderInterface *renderer) {
    materialBuffer = renderer->createUniformBuffer(
        &materialSettings, sizeof(materialSettings),
        APITest::MemoryType::GPU_PRIVATE);
  }
  void compileDescriptors() {
    if (!sampler)
      throw std::runtime_error("[MESH][ERROR] no sampler assigned to material");
    textureDescriptors.clear();
#define BIND_TEXTURE(TEXTURE, BINDING)                                         \
  if (TEXTURE) {                                                               \
    textureDescriptors.push_back(                                              \
        {BINDING, APITest::ShaderStage::FRAGMENT,                              \
         APITest::CombinedImageSampler{*sampler, *TEXTURE}});                  \
    materialUniformLayout.push_back(                                           \
        {BINDING, APITest::ShaderStage::FRAGMENT,                              \
         APITest::DescriptorLayout::Type::COMBINED_IMAGE_SAMPLER});            \
  }

    BIND_TEXTURE(baseColorTexture, 2);
    BIND_TEXTURE(metallicRoughnessTexture, 3);
    BIND_TEXTURE(normalTexture, 4);
    BIND_TEXTURE(occlusionTexture, 5);
    BIND_TEXTURE(emissiveTexture, 6);
    BIND_TEXTURE(specularGlossinessTexture, 7);
    BIND_TEXTURE(diffuseTexture, 8);
    textureDescriptors.push_back(
        {9, APITest::ShaderStage::FRAGMENT, materialBuffer});
    materialUniformLayout.push_back(
        {9, APITest::ShaderStage::FRAGMENT,
         APITest::DescriptorLayout::Type::UNIFORM_BUFFER});
#undef BIND_TEXTURE
  }

  virtual std::vector<APITest::DescriptorLayout>
  getDescriptorLayout() const override {
    return materialUniformLayout;
  }
  std::vector<APITest::UniformDescriptor> getDescriptors() const override {
    return textureDescriptors;
  }

  APITest::ShaderRef getFragmentShader() const override {
    return fragmentShader;
  };
};

class MMesh : public MeshBase {
  std::vector<MaterialRef *> materials;
  std::vector<
      std::pair<APITest::PipelineRef,
                std::pair<APITest::DescriptorSetLayoutRef,
                          std::map<size_t, APITest::UniformDescriptorSetRef>>>>
      perPrimitivePipeline;

public:
  MMesh(APITest::RenderInterface *renderer, tinygltf::Model const &model,
        tinygltf::Node const &node, std::vector<MaterialRef *> materials);

  void resetPipelines(APITest::RenderInterface *renderer);

  void drawAll(APITest::CommandRecorder *commandRecorder) const;

  void drawInstance(APITest::CommandRecorder *commandRecorder,
                    size_t instanceID) const;

  void addNewInstance(size_t instanceId,
                      APITest::UniformDescriptor const &transformDescriptor,
                      APITest::UniformDescriptor const &cameraDescriptor);

  void eraseInstance(size_t instanceId);

  ~MMesh() override = default;
};

struct MNode {
  size_t index;
  std::weak_ptr<MNode> parent;
  std::vector<std::shared_ptr<MNode>> children;
  std::unique_ptr<MMesh> mesh;

  std::string name;

  struct InstanceData {
    glm::mat4 transform;
  };

  // this fields are used to store initial data of a Node
  // and not used by actual instances

  InstanceData initialData;

  // And this array stores actual instance data

  std::map<size_t, std::pair<InstanceData, APITest::UniformBufferRef>>
      instanceBuffers;
};

class GLTFModelInstance;

class GLTFModel {
  std::vector<MaterialRef> materials;
  std::vector<APITest::ImageRef> textures;
  std::vector<std::shared_ptr<MNode>> rootNodes;
  std::vector<std::shared_ptr<MNode>> linearNodes;
  static APITest::RenderInterface *renderer_;
  Camera &camera_;
  APITest::SamplerRef sampler;
  std::stack<size_t> freeIDs;
  size_t instanceCount = 0;

  APITest::ImageRef *getTexture(int index) {
    if (index >= 0 && index < textures.size())
      return &textures.at(index);
    return nullptr;
  }
  void loadImages(tinygltf::Model &gltfModel);
  void loadMaterials(tinygltf::Model &gltfModel);
  void loadNode(tinygltf::Model &gltfModel, std::shared_ptr<MNode> parent,
                const tinygltf::Node &node, uint32_t nodeIndex);

  void destroyInstance(size_t id);
  void drawInstance(APITest::CommandRecorder *recorder, size_t id);

public:
  static void Init(APITest::RenderInterfaceRef &renderer) {
    renderer_ = renderer.get();
    MMaterial::InitShader(renderer);
    MaterialBase::InitMaterialBase(renderer_);
  }
  GLTFModel(std::filesystem::path const &path, Camera &camera);

  GLTFModelInstance createNewInstance();

  friend class GLTFModelInstance;
};

class GLTFModelInstance {
  GLTFModel *model_;
  std::optional<size_t> id_;
  GLTFModelInstance(GLTFModel *model, size_t id) : model_(model), id_(id) {}

public:
  GLTFModelInstance(GLTFModelInstance const &another) = delete;
  GLTFModelInstance(GLTFModelInstance &&another) noexcept {
    model_ = another.model_;
    id_ = another.id_;
    another.id_.reset();
  };

  GLTFModelInstance const &operator=(GLTFModelInstance const &another) = delete;
  GLTFModelInstance &operator=(GLTFModelInstance &&another) noexcept {
    model_ = another.model_;
    id_ = another.id_;
    another.id_.reset();
    return *this;
  }
  void draw(APITest::CommandRecorder *recorder) {
    model_->drawInstance(recorder, id_.value());
  };

  ~GLTFModelInstance();

  friend class GLTFModel;
};

} // namespace Examples
#endif // RENDERAPITEST_MODEL_H
