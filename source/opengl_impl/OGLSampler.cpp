//
// Created by Бушев Дмитрий on 16.11.2021.
//

#include "OGLSampler.h"
#include "OGLDebug.h"

APITest::OGLSampler::OGLSampler(const APITest::SamplerDesc &desc) {
  glGenSamplers(1, &sampler_);

  glSamplerParameteri(sampler_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glSamplerParameteri(sampler_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  checkGLError();
}

APITest::OGLSampler::~OGLSampler() { glDeleteSamplers(1, &sampler_); }

void APITest::OGLSampler::bind(GLuint texture) const {
  glBindSampler(texture, sampler_);
}
