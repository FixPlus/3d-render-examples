//
// Created by Бушев Дмитрий on 11.11.2021.
//

#ifndef RENDERAPITEST_OGLBUFFER_H
#define RENDERAPITEST_OGLBUFFER_H

#include "RenderInterface.h"
#include <GL/glew.h>

namespace APITest {

class OGLBuffer : virtual BufferBase {
  GLuint buffer_ = 0;
  GLenum type_;
  size_t size_;
  MemoryType memType_;

public:
  GLuint buffer() const { return buffer_; }
  OGLBuffer(GLenum type, size_t size, MemoryType memType);
  size_t size() const override { return size_; };
  void push(const void *data, size_t size, size_t offset) override;

  ~OGLBuffer() override;
};

class OGLVertexBuffer : public VertexBuffer, public OGLBuffer {

public:
  OGLVertexBuffer(MemoryType memType, size_t size)
      : OGLBuffer(GL_ARRAY_BUFFER, size, memType){};
};

class OGLIndexBuffer : public IndexBuffer, public OGLBuffer {
  Type indType_;

public:
  GLenum indexType() const {
    switch (indType_) {
    case Type::INDEX_TYPE_UINT_32:
      return GL_UNSIGNED_INT;
    case Type::INDEX_TYPE_UINT_16:
      return GL_UNSIGNED_SHORT;
    case Type::INDEX_TYPE_UINT_8:
      return GL_UNSIGNED_BYTE;
    default:
      return 0;
    }
  }

  OGLIndexBuffer(MemoryType memType, size_t size, Type indType)
      : OGLBuffer(GL_ELEMENT_ARRAY_BUFFER, size, memType), indType_(indType){};
};

struct OGLUniformBuffer : public UniformBuffer, public OGLBuffer {

  void bind(uint32_t binding) const;

  OGLUniformBuffer(MemoryType memType, size_t size)
      : OGLBuffer(GL_UNIFORM_BUFFER, size, memType){};
};

} // namespace APITest
#endif // RENDERAPITEST_OGLBUFFER_H
