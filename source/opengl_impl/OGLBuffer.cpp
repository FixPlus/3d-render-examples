//
// Created by Бушев Дмитрий on 11.11.2021.
//

#include "OGLBuffer.h"
#include "OGLDebug.h"
#include <cstring>
#include <string>

void APITest::OGLBuffer::push(const void *data, size_t size, size_t offset) {
  if (size_ < size + offset)
    throw std::runtime_error(
        "[OGL][ERROR] trying to push data out of Vertex Buffer bounds: " +
        std::to_string(size + offset - size_) + " bytes overflow.");

  glBindBuffer(type_, buffer_);

  auto *mapped = glMapBuffer(type_, GL_WRITE_ONLY);

  checkGLError();

  memcpy((char *)mapped + offset, data, size);

  glUnmapBuffer(type_);

  checkGLError();
}

APITest::OGLBuffer::OGLBuffer(GLenum type, size_t size, MemoryType memType)
    : type_(type), size_(size), memType_(memType) {
  glGenBuffers(1, &buffer_);
  checkGLError();

  glBindBuffer(type_, buffer_);
  glBufferData(type_, size, nullptr,
               memType == MemoryType::GPU_PRIVATE ? GL_STATIC_DRAW
                                                  : GL_DYNAMIC_DRAW);
  checkGLError();
}

APITest::OGLBuffer::~OGLBuffer() { glDeleteBuffers(1, &buffer_); }

void APITest::OGLUniformBuffer::bind(uint32_t binding) const {
  glBindBuffer(GL_UNIFORM_BUFFER, buffer());
  glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer());
  checkGLError();
}
