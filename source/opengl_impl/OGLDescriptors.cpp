//
// Created by Бушев Дмитрий on 16.11.2021.
//

#include "OGLDescriptors.h"
#include "OGLBuffer.h"
#include "OGLSampler.h"
#include "OGLTexture.h"
#include <GL/glew.h>
#include <stdexcept>

APITest::UniformDescriptorSetRef
APITest::OGLDescriptorSetLayout::allocateNewSet(
    APITest::UniformDescriptor *descriptors, int count) {
  return UniformDescriptorSetRef(new OGLDescriptorSet(descriptors, count));
}

void APITest::OGLDescriptorSet::bind() const {

  for (auto &desc : descriptors) {
    if (std::holds_alternative<CombinedImageSampler>(desc.descriptor)) {
      auto combSampler = std::get<CombinedImageSampler>(desc.descriptor);
      glActiveTexture(GL_TEXTURE0 + desc.binding);
      auto *texture = dynamic_cast<OGLTexture *>(combSampler.second.get());
      auto *sampler = dynamic_cast<OGLSampler *>(combSampler.first.get());
      sampler->bind(desc.binding);
      texture->bind();
    } else {
      auto uniformBuffer = std::get<UniformBufferRef>(desc.descriptor);
      auto *bufferImpl = dynamic_cast<OGLUniformBuffer *>(uniformBuffer.get());
      if (!bufferImpl)
        throw std::runtime_error("[OGL][ERROR]: invalid UniformBuffer bound");

      bufferImpl->bind(desc.binding);
    }
  }
}
