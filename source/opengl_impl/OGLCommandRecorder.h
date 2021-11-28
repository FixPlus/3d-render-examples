//
// Created by Бушев Дмитрий on 09.11.2021.
//

#ifndef RENDERAPITEST_OGLCOMMANDRECORDER_H
#define RENDERAPITEST_OGLCOMMANDRECORDER_H

#include "RenderInterface.h"
#include <GL/glew.h>

namespace APITest {

struct OGLPipeline;
struct OGLCommandRecorder final : public CommandRecorder {
  OGLPipeline *currentPipeline = nullptr;
  GLenum currentIndexType = GL_UNSIGNED_INT;
  void bindPipeline(Pipeline *pipeline) override;
  void bindVertexBuffer(VertexBuffer *, uint32_t binding) override;
  void bindIndexBuffer(IndexBuffer *buffer) override;
  void draw(uint32_t vertexCount, uint32_t firstIndex) override;
  void bindDescriptorSet(UniformDescriptorSet *set) override;
  void drawIndexed(uint32_t indexCount, uint32_t firstIndex,
                   uint32_t vertexOffset) override;
};

} // namespace APITest
#endif // RENDERAPITEST_OGLCOMMANDRECORDER_H
