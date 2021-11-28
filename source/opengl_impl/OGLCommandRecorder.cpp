//
// Created by Бушев Дмитрий on 09.11.2021.
//

#include "OGLCommandRecorder.h"
#include "OGLBuffer.h"
#include "OGLDescriptors.h"
#include "OGLPipeline.h"

#include <cassert>
#include <stdexcept>

void APITest::OGLCommandRecorder::bindPipeline(APITest::Pipeline *pipeline) {
  if (auto *oglPipeline = dynamic_cast<OGLPipeline *>(pipeline)) {
    oglPipeline->bind();
    currentPipeline = oglPipeline;
  } else {
    throw std::runtime_error(
        "[OGL][Error]: bindPipeline() - invalid pipeline object passed.");
  }
}

void APITest::OGLCommandRecorder::draw(uint32_t vertexCount,
                                       uint32_t firstIndex) {
  glDrawArrays(GL_TRIANGLES, firstIndex, vertexCount);
}

static std::pair<GLint, GLenum>
parseVertexAttr(APITest::VertexLayout::Attribute attr) {
  GLint size;
  GLenum type;
  switch (attr) {
  case APITest::VertexLayout::Attribute::RGBA32SF:
    size = 4, type = GL_FLOAT;
    break;
  case APITest::VertexLayout::Attribute::RGBA8UNORM:
    size = 4, type = GL_UNSIGNED_BYTE;
    break;
  case APITest::VertexLayout::Attribute::RGB32SF:
    size = 3, type = GL_FLOAT;
    break;
  case APITest::VertexLayout::Attribute::RG32SF:
    size = 2, type = GL_FLOAT;
    break;
  case APITest::VertexLayout::Attribute::R32SF:
    size = 1, type = GL_FLOAT;
    break;
  case APITest::VertexLayout::Attribute::MAT4F:
    size = 16, type = GL_FLOAT;
    break;
  }

  return {size, type};
}

inline GLint attrSize(APITest::VertexLayout::Attribute attr) {
  if (attr == APITest::VertexLayout::Attribute::RGBA8UNORM) {
    return 1;
  } else {
    return 4;
  }
}

inline GLint indexTypeSize(GLenum indexType) {
  switch (indexType) {
  case GL_UNSIGNED_INT:
    return 4;
  case GL_UNSIGNED_SHORT:
    return 2;
  case GL_UNSIGNED_BYTE:
    return 1;
  default:
    return 0;
  }
}

static bool normalizedFormat(APITest::VertexLayout::Attribute attr) {
  return attr == APITest::VertexLayout::Attribute::RGBA8UNORM;
}

void APITest::OGLCommandRecorder::bindVertexBuffer(
    APITest::VertexBuffer *buffer, uint32_t binding) {
  auto *graphicsPipeline = dynamic_cast<OGLGraphicsPipeline *>(currentPipeline);
  if (!graphicsPipeline)
    throw std::runtime_error("[OGL][ERROR]: trying to bind Vertex Buffer when "
                             "no graphics pipeline has been bound");

  auto *bufImpl = dynamic_cast<OGLVertexBuffer *>(buffer);

  if (!bufImpl)
    throw std::runtime_error(
        "[OGL][ERROR]: invalid vertex/index buffer passed to command recorder");

  auto glBuf = bufImpl->buffer();

  auto const &layout = graphicsPipeline->layout();

  glBindBuffer(GL_ARRAY_BUFFER, glBuf);

  GLuint index = 0;
  GLint stride = 0;
  GLint offset = 0;

  for (auto attr : layout.vertexLayout.perVertexAttribute)
    stride += parseVertexAttr(attr).first * attrSize(attr);
  for (auto attr : layout.vertexLayout.perVertexAttribute) {
    auto [size, type] = parseVertexAttr(attr);
    int locCount = 1;
    if (size > 4) {
      assert(size % 4 && "Extra size must be 16 bytes aligned");
      locCount = size / 4;
      size = 4;
    }
    for (int i = 0; i < locCount; ++i) {
      glVertexAttribPointer(index, size, type,
                            normalizedFormat(attr) ? GL_TRUE : GL_FALSE, stride,
                            (void *)(offset));
      glEnableVertexAttribArray(index);
      offset += size * attrSize(attr);
      index++;
    }
  }
}

void APITest::OGLCommandRecorder::bindDescriptorSet(
    APITest::UniformDescriptorSet *set) {
  if (auto *oglDesc = dynamic_cast<OGLDescriptorSet *>(set)) {
    oglDesc->bind();
  } else {
    throw std::runtime_error(
        "[OGL][ERROR] bindDescriptorSet(): invalid descriptor.");
  }
}

void APITest::OGLCommandRecorder::drawIndexed(uint32_t indexCount,
                                              uint32_t firstIndex,
                                              uint32_t vertexOffset) {
  auto offset = static_cast<GLint>(firstIndex);
  auto vOffset = static_cast<GLint>(vertexOffset);
  glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, currentIndexType,
                           (void *)(offset * indexTypeSize(currentIndexType)),
                           vOffset);
}

void APITest::OGLCommandRecorder::bindIndexBuffer(
    APITest::IndexBuffer *buffer) {
  auto *graphicsPipeline = dynamic_cast<OGLGraphicsPipeline *>(currentPipeline);
  if (!graphicsPipeline)
    throw std::runtime_error("[OGL][ERROR]: trying to bind Index Buffer when "
                             "no graphics pipeline has been bound");

  auto *bufImpl = dynamic_cast<OGLIndexBuffer *>(buffer);

  if (!bufImpl)
    throw std::runtime_error(
        "[OGL][ERROR]: invalid index buffer passed to command recorder");

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufImpl->buffer());
  currentIndexType = bufImpl->indexType();
}
