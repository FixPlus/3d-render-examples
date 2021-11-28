//
// Created by Бушев Дмитрий on 16.11.2021.
//

#ifndef RENDERAPITEST_OGLSAMPLER_H
#define RENDERAPITEST_OGLSAMPLER_H

#include "RenderInterface.h"
#include <GL/glew.h>

namespace APITest {

class OGLSampler : public Sampler {
  GLuint sampler_ = 0;

public:
  explicit OGLSampler(SamplerDesc const &desc);

  void bind(GLuint texture) const;
  ~OGLSampler() override;
};

} // namespace APITest
#endif // RENDERAPITEST_OGLSAMPLER_H
