//
// Created by Бушев Дмитрий on 20.11.2021.
//

#include "Model.h"
#include "common.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Examples {
APITest::ShaderRef MaterialBase::defaultMaterialShader = 0;
APITest::ShaderRef MMaterial::fragmentShader = 0;
APITest::RenderInterface *GLTFModel::renderer_ = nullptr;

void MMaterial::InitShader(const APITest::RenderInterfaceRef &renderer) {
  if (!fragmentShader)
    fragmentShader = renderer->createShaderProgram(
        {APITest::isVulkan(renderer.get())
             ? ROOT_SHADERS_DIR "common/gltf.frag.spv"
             : ROOT_SHADERS_DIR "common/gltf.frag",
         APITest::ShaderStage::FRAGMENT});
}
} // namespace Examples
void Examples::MaterialBase::InitMaterialBase(
    APITest::RenderInterface *renderer) {
  defaultMaterialShader = renderer->createShaderProgram(
      {APITest::isVulkan(renderer) ? ROOT_SHADERS_DIR "common/default.frag.spv"
                                   : ROOT_SHADERS_DIR "common/default.frag",
       APITest::ShaderStage::FRAGMENT});
}

Examples::MeshBase::MeshBase(APITest::RenderInterface *renderer,
                             tinygltf::Model const &model,
                             tinygltf::Mesh const &mesh, MeshCreateFlags flags,
                             std::vector<AttributeType> attrMapping)
    : name_(mesh.name) {

  if (attrMapping.empty())
    for (int i = 0; i < static_cast<int>(AttributeType::LAST); ++i)
      attrMapping.push_back(static_cast<AttributeType>(i));

  size_t totalIndexCount = 0, totalVertexCount = 0;

  std::vector<char> vertexBuf;
  std::vector<uint32_t> indexBuf;
  size_t perVertexSize = 0;

  for (auto &attr : attrMapping) {
    switch (attr) {

    case AttributeType::NORMAL:
      perVertexSize += sizeof(glm::vec3);
      break;
    case AttributeType::UV:
      perVertexSize += sizeof(glm::vec2);
      break;
    case AttributeType::POSITION:
    case AttributeType::COLOR:
    case AttributeType::JOINT:
    case AttributeType::WEIGHT:
      perVertexSize += sizeof(glm::vec4);
      break;
    case AttributeType::LAST:
      break;
    }
  }

  for (auto &primitive : mesh.primitives) {

    auto &evalPrimitive = primitives_.emplace_back();

    evalPrimitive.firstVertex = totalVertexCount;
    evalPrimitive.firstIndex = totalIndexCount;

    evalPrimitive.vertexCount =
        model.accessors[primitive.attributes.find("POSITION")->second].count;

    auto indexRef = primitive.indices;
    if (indexRef >= 0)
      evalPrimitive.indexCount = model.accessors[indexRef].count;
    else
      evalPrimitive.indexCount = 0;

    totalIndexCount += evalPrimitive.indexCount;
    totalVertexCount += evalPrimitive.vertexCount;

    vertexBuf.resize(perVertexSize * totalVertexCount);
    indexBuf.reserve(totalIndexCount);

    const float *bufferPos = nullptr;
    const float *bufferNormals = nullptr;
    const float *bufferTexCoords = nullptr;
    const float *bufferColors = nullptr;

    int numColorComponents;

    const tinygltf::Accessor &posAccessor =
        model.accessors[primitive.attributes.find("POSITION")->second];
    const tinygltf::BufferView &posView =
        model.bufferViews[posAccessor.bufferView];
    bufferPos = reinterpret_cast<const float *>(
        &(model.buffers[posView.buffer]
              .data[posAccessor.byteOffset + posView.byteOffset]));
    auto posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1],
                            posAccessor.minValues[2]);
    auto posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1],
                            posAccessor.maxValues[2]);

    evalPrimitive.setDimensions(posMin, posMax);
    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
      const tinygltf::Accessor &normAccessor =
          model.accessors[primitive.attributes.find("NORMAL")->second];
      const tinygltf::BufferView &normView =
          model.bufferViews[normAccessor.bufferView];
      bufferNormals = reinterpret_cast<const float *>(
          &(model.buffers[normView.buffer]
                .data[normAccessor.byteOffset + normView.byteOffset]));
    }

    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
      const tinygltf::Accessor &uvAccessor =
          model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
      const tinygltf::BufferView &uvView =
          model.bufferViews[uvAccessor.bufferView];
      bufferTexCoords = reinterpret_cast<const float *>(
          &(model.buffers[uvView.buffer]
                .data[uvAccessor.byteOffset + uvView.byteOffset]));
    }

    if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
      const tinygltf::Accessor &colorAccessor =
          model.accessors[primitive.attributes.find("COLOR_0")->second];
      const tinygltf::BufferView &colorView =
          model.bufferViews[colorAccessor.bufferView];
      // Color buffer are either of type vec3 or vec4
      numColorComponents =
          colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
      bufferColors = reinterpret_cast<const float *>(
          &(model.buffers[colorView.buffer]
                .data[colorAccessor.byteOffset + colorView.byteOffset]));
    }

    for (size_t i = 0; i < evalPrimitive.vertexCount; ++i) {
      size_t vertexOffset = (evalPrimitive.firstVertex + i) * perVertexSize;
      size_t localOffset = 0;
      for (auto &attr : attrMapping) {
        switch (attr) {
        case AttributeType::POSITION: {
          glm::vec4 pos = glm::vec4(glm::make_vec3(bufferPos + i * 3), 1.0f);
          memcpy(vertexBuf.data() + vertexOffset + localOffset, &pos,
                 sizeof(glm::vec4));
          localOffset += sizeof(glm::vec4);
          break;
        }
        case AttributeType::NORMAL: {
          if (bufferNormals)
            memcpy(vertexBuf.data() + vertexOffset + localOffset,
                   bufferNormals + i * 3, sizeof(glm::vec3));
          localOffset += sizeof(glm::vec3);
          break;
        }
        case AttributeType::UV: {
          if (bufferTexCoords)
            memcpy(vertexBuf.data() + vertexOffset + localOffset,
                   bufferTexCoords + i * 2, sizeof(glm::vec2));
          localOffset += sizeof(glm::vec2);
          break;
        }
        case AttributeType::COLOR: {

          if (bufferColors) {
            glm::vec4 color = glm::vec4(0.0f);
            color = numColorComponents == 4
                        ? glm::make_vec4(bufferColors + i * 4)
                        : glm::vec4(glm::make_vec3(bufferColors + i * 3), 0.0f);
            memcpy(vertexBuf.data() + vertexOffset + localOffset, &color,
                   sizeof(glm::vec4));
          }
          localOffset += sizeof(glm::vec4);
          break;
        }
        default:
          break;
        }
      }
    }

    if (evalPrimitive.indexCount > 0) {
      const tinygltf::Accessor &accessor = model.accessors[primitive.indices];
      const tinygltf::BufferView &bufferView =
          model.bufferViews[accessor.bufferView];
      const tinygltf::Buffer &buffer = model.buffers[bufferView.buffer];

      switch (accessor.componentType) {
      case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
        auto *buf = new uint32_t[accessor.count];
        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
               accessor.count * sizeof(uint32_t));
        for (size_t index = 0; index < accessor.count; index++) {
          indexBuf.push_back(buf[index]);
        }
        break;
      }
      case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
        auto *buf = new uint16_t[accessor.count];
        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
               accessor.count * sizeof(uint16_t));
        for (size_t index = 0; index < accessor.count; index++) {
          indexBuf.push_back(buf[index]);
        }
        break;
      }
      case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
        auto *buf = new uint8_t[accessor.count];
        memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
               accessor.count * sizeof(uint8_t));
        for (size_t index = 0; index < accessor.count; index++) {
          indexBuf.push_back(buf[index]);
        }
        break;
      }
      default:
        throw std::runtime_error(
            "[MESH][ERROR] loaded gltf model has invalid index type");
      }
    }
  }

  // TODO: implement vertex pretransformation here

  vertexBuffer = renderer->createVertexBuffer(
      vertexBuf.data(), vertexBuf.size(), APITest::MemoryType::GPU_PRIVATE);
  if (totalIndexCount)
    indexBuffer = renderer->createIndexBuffer(
        indexBuf.data(), totalIndexCount * 4,
        APITest::IndexBuffer::Type::INDEX_TYPE_UINT_32,
        APITest::MemoryType::GPU_PRIVATE);
}

void Examples::Primitive::setDimensions(glm::vec3 min, glm::vec3 max) {
  dimensions.min = min;
  dimensions.max = max;
  dimensions.size = max - min;
  dimensions.center = (min + max) / 2.0f;
  dimensions.radius = glm::distance(min, max) / 2.0f;
}

Examples::MMesh::MMesh(APITest::RenderInterface *renderer,
                       const tinygltf::Model &model, const tinygltf::Node &node,
                       std::vector<MaterialRef *> materials_)
    : MeshBase(renderer, model, model.meshes[node.mesh]),
      materials(std::move(materials_)) {

  resetPipelines(renderer);
}

void Examples::MMesh::resetPipelines(APITest::RenderInterface *renderer) {

  perPrimitivePipeline.clear();
  static APITest::VertexLayout vertexLayout{
      {APITest::VertexLayout::RGBA32SF, APITest::VertexLayout::RGB32SF,
       APITest::VertexLayout::RG32SF, APITest::VertexLayout::RGBA32SF,
       APITest::VertexLayout::RGBA32SF, APITest::VertexLayout::RGBA32SF},
      0};
  if (!vertexLayout.vertexShader)
    vertexLayout.vertexShader = renderer->createShaderProgram(
        {APITest::isVulkan(renderer) ? ROOT_SHADERS_DIR "common/gltf.vert.spv"
                                     : ROOT_SHADERS_DIR "common/gltf.vert",
         APITest::ShaderStage::VERTEX});

  assert(materials.size() == primitives_.size() &&
         "materials vector must match primitives vector");

  APITest::GraphicsPipelineLayout layout;
  layout.vertexLayout = vertexLayout;
  auto materialIt = materials.begin();
  auto materialEnd = materials.end();
  auto primitiveIt = primitives_.begin();
  std::vector<APITest::DescriptorLayout> uniforms;
  // camera binding point
  uniforms.push_back({0, APITest::ShaderStage::VERTEX,
                      APITest::DescriptorLayout::Type::UNIFORM_BUFFER});
  // transform matrix binding point
  uniforms.push_back({1, APITest::ShaderStage::VERTEX,
                      APITest::DescriptorLayout::Type::UNIFORM_BUFFER});

  // initialization of primitives pipelines
  for (; materialIt != materialEnd; ++materialIt, ++primitiveIt) {
    auto *material = (*materialIt)->get();
    layout.rasterizerLayout = material->getRasterizationState();
    layout.fragmentLayout.fragmentShader = material->getFragmentShader();
    layout.rasterizerLayout = material->getRasterizationState();
    auto materialDescriptors = material->getDescriptorLayout();
    uniforms.resize(2 + materialDescriptors.size());
    std::copy(materialDescriptors.begin(), materialDescriptors.end(),
              uniforms.begin() + 2);
    APITest::DescriptorSetLayoutRef primitiveLayout =
        renderer->createDescriptorLayout(uniforms);
    layout.descriptorsLayout = primitiveLayout;
    APITest::PipelineRef pipeline = renderer->createGraphicsPipeline(layout);
    perPrimitivePipeline.push_back(
        {std::move(pipeline),
         std::pair<APITest::DescriptorSetLayoutRef,
                   std::map<size_t, APITest::UniformDescriptorSetRef>>{
             primitiveLayout,
             std::map<size_t, APITest::UniformDescriptorSetRef>{}}});
  }
}

void Examples::MMesh::drawInstance(APITest::CommandRecorder *commandRecorder,
                                   size_t instanceID) const {
  int i = 0;
  for (auto const &primitive : perPrimitivePipeline) {
    auto const &instanceUniforms = primitive.second.second.at(instanceID);
    commandRecorder->bindPipeline(primitive.first.get());
    commandRecorder->bindDescriptorSet(instanceUniforms.get());
    bindBuffers(commandRecorder);
    drawPrimitive(commandRecorder, i);
    i++;
  }
}

void Examples::MMesh::drawAll(APITest::CommandRecorder *commandRecorder) const {
  if (perPrimitivePipeline.empty())
    return;
  int instanceCount = perPrimitivePipeline.front().second.second.size();

  for (int i = 0; i < instanceCount; ++i)
    drawInstance(commandRecorder, i);
}

void Examples::GLTFModel::loadImages(tinygltf::Model &gltfModel) {
  for (auto &image : gltfModel.images) {
    unsigned char *imageBuf = image.image.data();
    size_t bufferSize = image.width * image.height * 4;
    bool deleteBuffer = false;
    if (image.component == 3) {
      imageBuf = new unsigned char[bufferSize];
      unsigned char *rgba = imageBuf;
      unsigned char *rgb = &image.image[0];
      for (size_t i = 0; i < image.width * image.height; ++i) {
        for (int32_t j = 0; j < 3; ++j) {
          rgba[j] = rgb[j];
        }
        rgba += 4;
        rgb += 3;
      }
      deleteBuffer = true;
    }

    APITest::ImageDesc textureDesc;
    textureDesc.memType = APITest::MemoryType::GPU_PRIVATE;
    textureDesc.usage =
        APITest::Image::USAGE_COPY_TO | APITest::Image::USAGE_SAMPLED;
    textureDesc.extents.width = image.width;
    textureDesc.extents.height = image.height;
    textureDesc.extents.depth = 1;
    textureDesc.extents.layers = 1;
    textureDesc.type = APITest::Image::Type::TEXTURE_2D;
    textureDesc.format = APITest::Image::Format::RGBA8;

    APITest::ImageRef texture = renderer_->createImage(textureDesc);

    texture->load(imageBuf);

    if (deleteBuffer)
      delete imageBuf;

    textures.push_back(texture);
  }
}

void Examples::MMesh::addNewInstance(
    size_t instanceId, APITest::UniformDescriptor const &transformDescriptor,
    APITest::UniformDescriptor const &cameraDescriptor) {

  std::vector<APITest::UniformDescriptor> uniforms;

  uniforms.push_back(cameraDescriptor);
  uniforms.push_back(transformDescriptor);
  int i = 0;
  for (auto &primitive : perPrimitivePipeline) {
    auto materialUniforms = materials.at(i)->get()->getDescriptors();
    uniforms.resize(2 + materialUniforms.size());
    std::copy(materialUniforms.begin(), materialUniforms.end(),
              uniforms.begin() + 2);
    auto descriptorSet = primitive.second.first->allocateNewSet(
        uniforms.data(), uniforms.size());
    primitive.second.second.emplace(instanceId, descriptorSet);
    ++i;
  }
}

void Examples::MMesh::eraseInstance(size_t instanceId) {
  for (auto &primitive : perPrimitivePipeline)
    primitive.second.second.erase(instanceId);
}

Examples::GLTFModel::GLTFModel(std::filesystem::path const &path,
                               Camera &camera)
    : camera_(camera) {

  if (!renderer_)
    throw std::runtime_error("[MODEL][ERROR] trying create model without "
                             "initializing context GLTFModel::Init()");

  if (!path.has_extension())
    throw std::runtime_error("[MODEL][ERROR] " + path.generic_string() +
                             " file has no extension.");

  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF gltfContext;

  std::string error, warning;

  bool fileLoaded;

  if (path.extension() == ".gltf")
    fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning,
                                               path.generic_string());
  else if (path.extension() == ".glb")
    fileLoaded = gltfContext.LoadBinaryFromFile(&gltfModel, &error, &warning,
                                                path.generic_string());
  else
    throw std::runtime_error(
        "[MODEL][ERROR] " + path.generic_string() +
        " file has incorrect extension. Supported extensions are: gltf, glb");

  if (!fileLoaded)
    throw std::runtime_error("[GLTF][ERROR] failed to load model from file " +
                             path.generic_string() + ". " + error);

  if (!warning.empty())
    std::cout << "[GLTF][WARNING]: " << warning << std::endl;

  sampler = renderer_->createSampler({});

  loadImages(gltfModel);

  loadMaterials(gltfModel);

  const tinygltf::Scene &scene =
      gltfModel
          .scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

  for (int nodeIndex : scene.nodes) {
    const tinygltf::Node node = gltfModel.nodes[nodeIndex];
    loadNode(gltfModel, nullptr, node, nodeIndex);
  }
}

void Examples::GLTFModel::loadMaterials(tinygltf::Model &gltfModel) {
  for (tinygltf::Material &mat : gltfModel.materials) {
    MMaterial material{renderer_};
    if (mat.values.find("baseColorTexture") != mat.values.end()) {
      material.baseColorTexture = getTexture(
          gltfModel.textures[mat.values["baseColorTexture"].TextureIndex()]
              .source);
    }
    // Metallic roughness workflow
    if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
      material.metallicRoughnessTexture = getTexture(
          gltfModel
              .textures[mat.values["metallicRoughnessTexture"].TextureIndex()]
              .source);
    }
    if (mat.values.find("roughnessFactor") != mat.values.end()) {
      material.materialSettings.roughnessFactor =
          static_cast<float>(mat.values["roughnessFactor"].Factor());
    }
    if (mat.values.find("metallicFactor") != mat.values.end()) {
      material.materialSettings.metallicFactor =
          static_cast<float>(mat.values["metallicFactor"].Factor());
    }
    if (mat.values.find("baseColorFactor") != mat.values.end()) {
      material.materialSettings.baseColorFactor =
          glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
    }
    if (mat.additionalValues.find("normalTexture") !=
        mat.additionalValues.end()) {
      material.normalTexture = getTexture(
          gltfModel
              .textures[mat.additionalValues["normalTexture"].TextureIndex()]
              .source);
    }
    if (mat.additionalValues.find("emissiveTexture") !=
        mat.additionalValues.end()) {
      material.emissiveTexture = getTexture(
          gltfModel
              .textures[mat.additionalValues["emissiveTexture"].TextureIndex()]
              .source);
    }
    if (mat.additionalValues.find("occlusionTexture") !=
        mat.additionalValues.end()) {
      material.occlusionTexture = getTexture(
          gltfModel
              .textures[mat.additionalValues["occlusionTexture"].TextureIndex()]
              .source);
    }
    if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
      tinygltf::Parameter param = mat.additionalValues["alphaMode"];
      if (param.string_value == "BLEND") {
        material.alphaMode = MMaterial::ALPHAMODE_BLEND;
      }
      if (param.string_value == "MASK") {
        material.alphaMode = MMaterial::ALPHAMODE_MASK;
      }
    }
    if (mat.additionalValues.find("alphaCutoff") !=
        mat.additionalValues.end()) {
      material.materialSettings.alphaCutoff =
          static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
    }

    material.sampler = &sampler;

    material.compileDescriptors();

    materials.push_back(std::make_unique<MMaterial>(material));
  }
  // Push a default material at the end of the list for meshes with no material
  // assigned
  materials.push_back(std::make_unique<MaterialBase>());
}

void Examples::GLTFModel::loadNode(tinygltf::Model &gltfModel,
                                   std::shared_ptr<MNode> parent,
                                   const tinygltf::Node &node,
                                   uint32_t nodeIndex) {

  std::shared_ptr<MNode> newNode = std::make_shared<MNode>();
  newNode->index = nodeIndex;
  newNode->parent = parent;
  newNode->name = node.name;

  // Generate local node matrix

  // if node transform stored in uniform matrix - decompose it
  if (node.matrix.size() == 16) {
    glm::vec4 dummyPer;
    glm::mat4 uniformMatrix = glm::make_mat4x4(node.matrix.data());
    newNode->initialData.transform = uniformMatrix;
  } else {
    glm::mat4 transform{1.0f};

    glm::vec3 translation = glm::vec3(0.0f);
    if (node.translation.size() == 3) {
      translation = glm::make_vec3(node.translation.data());
    }
    transform = glm::translate(transform, translation);

    glm::mat4 rotation = glm::mat4(1.0f);
    if (node.rotation.size() == 4) {
      glm::quat q = glm::make_quat(node.rotation.data());
      rotation = glm::mat4(q);
    }
    transform = transform * rotation;

    glm::vec3 scale = glm::vec3(1.0f);
    if (node.scale.size() == 3) {
      scale = glm::make_vec3(node.scale.data());
    }
    transform = glm::scale(transform, scale);
    newNode->initialData.transform = transform;
  }

  // Node with children
  if (!node.children.empty()) {
    for (int i : node.children) {
      loadNode(gltfModel, newNode, gltfModel.nodes[i], i);
    }
  }

  if (node.mesh > -1) {
    std::vector<MaterialRef *> primitiveMaterials;
    for (auto &prim : gltfModel.meshes[node.mesh].primitives) {
      auto matIndex = prim.material;
      if (matIndex > -1)
        primitiveMaterials.emplace_back(materials.data() + matIndex);
      else
        primitiveMaterials.emplace_back(&materials.back()); // default material
    }

    newNode->mesh =
        std::make_unique<MMesh>(renderer_, gltfModel, node, primitiveMaterials);
  }

  if (parent) {
    parent->children.push_back(newNode);
  } else {
    rootNodes.push_back(newNode);
  }
  linearNodes.push_back(newNode);
}

Examples::GLTFModelInstance Examples::GLTFModel::createNewInstance() {
  size_t id;
  if (!freeIDs.empty()) {
    id = freeIDs.top();
    freeIDs.pop();
  } else {
    id = instanceCount;
  }

  instanceCount++;

  for (auto &node : linearNodes) {
    MNode::InstanceData data = node->initialData;

    auto [buffer, emplaced] = node->instanceBuffers.emplace(
        id, std::pair<MNode::InstanceData, APITest::UniformBufferRef>{data,
                                                                      nullptr});
    buffer->second.second = renderer_->createUniformBuffer(
        &buffer->second.first, sizeof(MNode::InstanceData));

    APITest::UniformDescriptor bufDesc = {1, APITest::ShaderStage::VERTEX,
                                          buffer->second.second};
    APITest::UniformDescriptor cameraDesc = {0, APITest::ShaderStage::VERTEX,
                                             camera_.getCameraBuffer()};
    if (node->mesh) {
      node->mesh->addNewInstance(id, bufDesc, cameraDesc);
    }
  }

  return GLTFModelInstance(this, id);
}

void Examples::GLTFModel::destroyInstance(size_t id) {
  instanceCount--;
  if (id != instanceCount) {
    freeIDs.push(id);
  }

  for (auto &node : linearNodes) {
    node->instanceBuffers.erase(id);
    if (node->mesh)
      node->mesh->eraseInstance(id);
  }
}

void Examples::GLTFModel::drawInstance(APITest::CommandRecorder *recorder,
                                       size_t id) {
  for (auto &node : linearNodes) {
    if (node->mesh)
      node->mesh->drawInstance(recorder, id);
  }
}

Examples::GLTFModelInstance::~GLTFModelInstance() {
  if (id_)
    model_->destroyInstance(id_.value());
}
